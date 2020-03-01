/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "TemporaryFile.h"

#include <QTextStream>

TemporaryFile::TemporaryFile()
    : TemporaryFile(nullptr)
{
}

TemporaryFile::TemporaryFile(const QString& templateName)
    : TemporaryFile(templateName, nullptr)
{
}

TemporaryFile::TemporaryFile(QObject* parent)
    : QFile(parent)
{
    QTemporaryFile tmp;
    tmp.open();
    QFile::setFileName(tmp.fileName());
    tmp.close();
}

TemporaryFile::TemporaryFile(const QString& templateName, QObject* parent)
    : QFile(parent)
{
    QTemporaryFile tmp(templateName);
    tmp.open();
    QFile::setFileName(tmp.fileName());
    tmp.close();
}

TemporaryFile::~TemporaryFile()
{
    remove();
}

bool TemporaryFile::open()
{
    return QFile::open(QIODevice::ReadWrite);
}

bool TemporaryFile::copyFromFile(const QString& otherFileName)
{
    close();
    if (!open(QFile::WriteOnly | QFile::Truncate)) {
        return false;
    }

    QFile otherFile(otherFileName);
    if (!otherFile.open(QFile::ReadOnly)) {
        close();
        return false;
    }

    QByteArray data;
    while (!(data = otherFile.read(1024)).isEmpty()) {
        write(data);
    }

    otherFile.close();
    close();
    return true;
}
