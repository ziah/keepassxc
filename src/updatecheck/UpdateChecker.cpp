/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "UpdateChecker.h"

#include "config-keepassx.h"
#include "core/Clock.h"
#include "core/Config.h"
#include "core/NetworkManager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

UpdateChecker* UpdateChecker::m_instance(nullptr);

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_reply(nullptr)
    , m_isManuallyRequested(false)
{
}

UpdateChecker::~UpdateChecker()
{
}

void UpdateChecker::checkForUpdates(bool manuallyRequested)
{
    auto nextCheck = config()->get("GUI/CheckForUpdatesNextCheck", 0).toULongLong();
    m_isManuallyRequested = manuallyRequested;

    if (m_isManuallyRequested || Clock::currentSecondsSinceEpoch() >= nextCheck) {
        m_bytesReceived.clear();

        QString apiUrlStr = QString("https://api.github.com/repos/keepassxreboot/keepassxc/releases");

        if (!config()->get("GUI/CheckForUpdatesIncludeBetas", false).toBool()) {
            apiUrlStr += "/latest";
        }

        QUrl apiUrl = QUrl(apiUrlStr);

        QNetworkRequest request(apiUrl);
        request.setRawHeader("Accept", "application/json");

        m_reply = getNetMgr()->get(request);

        connect(m_reply, &QNetworkReply::finished, this, &UpdateChecker::fetchFinished);
        connect(m_reply, &QIODevice::readyRead, this, &UpdateChecker::fetchReadyRead);
    }
}

void UpdateChecker::fetchReadyRead()
{
    m_bytesReceived += m_reply->readAll();
}

void UpdateChecker::fetchFinished()
{
    bool error = (m_reply->error() != QNetworkReply::NoError);
    bool hasNewVersion = false;
    QUrl url = m_reply->url();
    QString version = "";

    m_reply->deleteLater();
    m_reply = nullptr;

    if (!error) {
        QJsonDocument jsonResponse = QJsonDocument::fromJson(m_bytesReceived);
        QJsonObject jsonObject = jsonResponse.object();

        if (config()->get("GUI/CheckForUpdatesIncludeBetas", false).toBool()) {
            QJsonArray jsonArray = jsonResponse.array();
            jsonObject = jsonArray.at(0).toObject();
        }

        if (!jsonObject.value("tag_name").isUndefined()) {
            version = jsonObject.value("tag_name").toString();
            hasNewVersion = compareVersions(QString(KEEPASSXC_VERSION), version);
        }

        // Check again in 7 days
        // TODO: change to toSecsSinceEpoch() when min Qt >= 5.8
        config()->set("GUI/CheckForUpdatesNextCheck", Clock::currentDateTime().addDays(7).toTime_t());
    } else {
        version = "error";
    }

    emit updateCheckFinished(hasNewVersion, version, m_isManuallyRequested);
}

bool UpdateChecker::compareVersions(const QString& localVersion, const QString& remoteVersion)
{
    // Quick full-string equivalence check
    if (localVersion == remoteVersion) {
        return false;
    }

    QRegularExpression verRegex(R"(^((?:\d+\.){2}\d+)(?:-(\w+?)(\d+)?)?$)");

    auto lmatch = verRegex.match(localVersion);
    auto rmatch = verRegex.match(remoteVersion);

    auto lVersion = lmatch.captured(1).split(".");
    auto lSuffix = lmatch.captured(2);
    auto lBetaNum = lmatch.captured(3);

    auto rVersion = rmatch.captured(1).split(".");
    auto rSuffix = rmatch.captured(2);
    auto rBetaNum = rmatch.captured(3);

    if (!lVersion.isEmpty() && !rVersion.isEmpty()) {
        if (lSuffix.compare("snapshot", Qt::CaseInsensitive) == 0) {
            // Snapshots are not checked for version updates
            return false;
        }

        // Check "-beta[X]" versions
        if (lVersion == rVersion && !lSuffix.isEmpty()) {
            // Check if stable version has been released or new beta is available
            // otherwise the version numbers are equal
            return rSuffix.isEmpty() || lBetaNum.toInt() < rBetaNum.toInt();
        }

        for (int i = 0; i < 3; i++) {
            int l = lVersion[i].toInt();
            int r = rVersion[i].toInt();

            if (l == r) {
                continue;
            }

            if (l > r) {
                return false; // Installed version is newer than release
            } else {
                return true; // Installed version is outdated
            }
        }

        return false; // Installed version is the same
    }

    return false; // Invalid version string
}

UpdateChecker* UpdateChecker::instance()
{
    if (!m_instance) {
        m_instance = new UpdateChecker();
    }

    return m_instance;
}
