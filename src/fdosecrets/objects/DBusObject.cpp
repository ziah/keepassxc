/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "DBusObject.h"

#include <QDBusAbstractAdaptor>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QUrl>
#include <QUuid>

#include <utility>

namespace FdoSecrets
{

    DBusObject::DBusObject(DBusObject* parent)
        : QObject(parent)
    {
    }

    void DBusObject::registerWithPath(const QString& path, QDBusAbstractAdaptor* adaptor)
    {
        m_objectPath.setPath(path);
        m_dbusAdaptor = adaptor;
        adaptor->setParent(this);
        auto ok = QDBusConnection::sessionBus().registerObject(m_objectPath.path(), this);
        Q_UNUSED(ok);
        Q_ASSERT(ok);
    }

    QString DBusObject::callingPeerName() const
    {
        auto pid = callingPeerPid();
        QFile proc(QStringLiteral("/proc/%1/comm").arg(pid));
        if (!proc.open(QFile::ReadOnly)) {
            return callingPeer();
        }
        QTextStream stream(&proc);
        return stream.readAll().trimmed();
    }

    QString encodePath(const QString& value)
    {
        // force "-.~_" to be encoded
        return QUrl::toPercentEncoding(value, "", "-.~_").replace('%', '_');
    }

} // namespace FdoSecrets
