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

#ifndef KEEPASSXC_TEMPORARYFILE_H
#define KEEPASSXC_TEMPORARYFILE_H

#include <QTemporaryFile>

class TemporaryFile : public QFile
{
    Q_OBJECT

public:
    TemporaryFile();
    explicit TemporaryFile(const QString& templateName);
    explicit TemporaryFile(QObject* parent);
    TemporaryFile(const QString& templateName, QObject* parent);
    ~TemporaryFile() override;

    using QFile::open;
    bool open();
    bool copyFromFile(const QString& otherFileName);
};

#endif // KEEPASSXC_TEMPORARYFILE_H
