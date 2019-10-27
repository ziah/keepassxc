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

#include "Open.h"

#include <QCommandLineParser>

#include "DatabaseCommand.h"
#include "TextStream.h"
#include "Utils.h"

Open::Open()
{
    name = QString("open");
    description = QObject::tr("Open a database.");
}

int Open::execute(const QStringList& arguments)
{
    currentDatabase.reset(nullptr);
    return this->DatabaseCommand::execute(arguments);
}

int Open::executeWithDatabase(QSharedPointer<Database> db, QSharedPointer<QCommandLineParser> parser)
{
    Q_UNUSED(parser)
    currentDatabase = db;
    return EXIT_SUCCESS;
}
