/*
 *  Copyright (C) 2014 Kyle Manna <kyle@kylemanna.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include <ykcore.h>
#include <ykdef.h>
#include <ykpers-version.h>
#include <ykstatus.h>
#include <yubikey.h>

#include "core/Global.h"
#include "core/Tools.h"
#include "crypto/Random.h"

#include "YubiKey.h"

// Cast the void pointer from the generalized class definition
// to the proper pointer type from the now included system headers
#define m_yk (static_cast<YK_KEY*>(m_yk_void))
#define m_ykds (static_cast<YK_STATUS*>(m_ykds_void))

YubiKey::YubiKey()
    : m_yk_void(nullptr)
    , m_ykds_void(nullptr)
    , m_onlyKey(false)
    , m_mutex(QMutex::Recursive)
{
}

YubiKey* YubiKey::m_instance(Q_NULLPTR);

YubiKey* YubiKey::instance()
{
    if (!m_instance) {
        m_instance = new YubiKey();
    }

    return m_instance;
}

bool YubiKey::init()
{
    m_mutex.lock();

    // previously initialized
    if (m_yk != nullptr && m_ykds != nullptr) {

        if (yk_get_status(m_yk, m_ykds)) {
            // Still connected
            m_mutex.unlock();
            return true;
        } else {
            // Initialized but not connected anymore, re-init
            deinit();
        }
    }

    if (!yk_init()) {
        m_mutex.unlock();
        return false;
    }

    // TODO: handle multiple attached hardware devices
    m_onlyKey = false;
    m_yk_void = static_cast<void*>(yk_open_first_key());
#if YKPERS_VERSION_NUMBER >= 0x011400
    // New fuction available in yubikey-personalization version >= 1.20.0 that allows
    // selecting device VID/PID (yk_open_key_vid_pid)
    if (m_yk == nullptr) {
        static const int device_pids[] = {0x60fc}; // OnlyKey PID
        m_yk_void = static_cast<void*>(yk_open_key_vid_pid(0x1d50, device_pids, 1, 0));
        m_onlyKey = true;
    }
#endif
    if (m_yk == nullptr) {
        yk_release();
        m_mutex.unlock();
        return false;
    }

    m_ykds_void = static_cast<void*>(ykds_alloc());
    if (m_ykds == nullptr) {
        yk_close_key(m_yk);
        m_yk_void = nullptr;
        yk_release();
        m_mutex.unlock();
        return false;
    }

    m_mutex.unlock();
    return true;
}

bool YubiKey::deinit()
{
    m_mutex.lock();

    if (m_yk) {
        yk_close_key(m_yk);
        m_yk_void = nullptr;
    }

    if (m_ykds) {
        ykds_free(m_ykds);
        m_ykds_void = nullptr;
    }

    yk_release();

    m_mutex.unlock();

    return true;
}

void YubiKey::detect()
{
    bool found = false;

    // Check slot 1 and 2 for Challenge-Response HMAC capability
    for (int i = 1; i <= 2; ++i) {
        QString errorMsg;
        bool isBlocking = checkSlotIsBlocking(i, errorMsg);
        if (errorMsg.isEmpty()) {
            found = true;
            emit detected(i, isBlocking);
        }

        // Wait between slots to let the yubikey settle.
        Tools::sleep(150);
    }

    if (!found) {
        emit notFound();
    } else {
        emit detectComplete();
    }
}

bool YubiKey::checkSlotIsBlocking(int slot, QString& errorMessage)
{
    if (!init()) {
        errorMessage = QString("Could not initialize YubiKey.");
        return false;
    }

    YubiKey::ChallengeResult result;
    QByteArray rand = randomGen()->randomArray(1);
    QByteArray resp;

    result = challenge(slot, false, rand, resp);
    if (result == ALREADY_RUNNING) {
        // Try this slot again after waiting
        Tools::sleep(300);
        result = challenge(slot, false, rand, resp);
    }

    if (result == SUCCESS || result == WOULDBLOCK) {
        return result == WOULDBLOCK;
    } else if (result == ALREADY_RUNNING) {
        errorMessage = QString("YubiKey busy");
        return false;
    } else if (result == ERROR) {
        errorMessage = QString("YubiKey error");
        return false;
    }

    errorMessage = QString("Error while polling YubiKey");
    return false;
}

bool YubiKey::getSerial(unsigned int& serial)
{
    m_mutex.lock();
    int result = yk_get_serial(m_yk, 1, 0, &serial);
    m_mutex.unlock();

    if (!result) {
        return false;
    }

    return true;
}

QString YubiKey::getVendorName()
{
    return m_onlyKey ? "OnlyKey" : "YubiKey";
}

YubiKey::ChallengeResult YubiKey::challenge(int slot, bool mayBlock, const QByteArray& challenge, QByteArray& response)
{
    // ensure that YubiKey::init() succeeded
    if (!init()) {
        return ERROR;
    }

    int yk_cmd = (slot == 1) ? SLOT_CHAL_HMAC1 : SLOT_CHAL_HMAC2;
    QByteArray paddedChallenge = challenge;

    // yk_challenge_response() insists on 64 bytes response buffer */
    response.clear();
    response.resize(64);

    /* The challenge sent to the yubikey should always be 64 bytes for
     * compatibility with all configurations.  Follow PKCS7 padding.
     *
     * There is some question whether or not 64 bytes fixed length
     * configurations even work, some docs say avoid it.
     */
    const int padLen = 64 - paddedChallenge.size();
    if (padLen > 0) {
        paddedChallenge.append(QByteArray(padLen, padLen));
    }

    const unsigned char* c;
    unsigned char* r;
    c = reinterpret_cast<const unsigned char*>(paddedChallenge.constData());
    r = reinterpret_cast<unsigned char*>(response.data());

    // Try to grab a lock for 1 second, fail out if not possible
    if (!m_mutex.tryLock(1000)) {
        return ALREADY_RUNNING;
    }

    int ret = yk_challenge_response(m_yk, yk_cmd, mayBlock, paddedChallenge.size(), c, response.size(), r);
    m_mutex.unlock();

    if (!ret) {
        if (yk_errno == YK_EWOULDBLOCK) {
            return WOULDBLOCK;
        } else if (yk_errno == YK_ETIMEOUT) {
            return ERROR;
        } else if (yk_errno) {

            /* Something went wrong, close the key, so that the next call to
             * can try to re-open.
             *
             * Likely caused by the YubiKey being unplugged.
             */

            if (yk_errno == YK_EUSBERR) {
                qWarning("USB error: %s", yk_usb_strerror());
            } else {
                qWarning("YubiKey core error: %s", yk_strerror(yk_errno));
            }

            return ERROR;
        }
    }

    // actual HMAC-SHA1 response is only 20 bytes
    response.resize(20);

    return SUCCESS;
}
