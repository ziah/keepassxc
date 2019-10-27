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

#include <cstdlib>
#include <stdio.h>

#include "Export.h"

#include "cli/TextStream.h"
#include "cli/Utils.h"
#include "core/Database.h"
#include "format/CsvExporter.h"

const QCommandLineOption Export::FormatOption =
    QCommandLineOption(QStringList() << "f"
                                     << "format",
                       QObject::tr("Format to use when exporting. Available choices are xml or csv. Defaults to xml."),
                       QStringLiteral("xml|csv"));

Export::Export()
{
    name = QStringLiteral("export");
    options.append(Export::FormatOption);
    description = QObject::tr("Exports the content of a database to standard output in the specified format.");
}

int Export::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QString format = parser->value(Export::FormatOption);
    if (format.isEmpty() || format == QStringLiteral("xml")) {
        QByteArray xmlData;
        QString errorMessage;
        if (!database->extract(xmlData, &errorMessage)) {
            errorTextStream << QObject::tr("Unable to export database to XML: %1").arg(errorMessage) << endl;
            return EXIT_FAILURE;
        }
        outputTextStream << xmlData.constData() << endl;
    } else if (format == QStringLiteral("csv")) {
        CsvExporter csvExporter;
        outputTextStream << csvExporter.exportDatabase(database);
    } else {
        errorTextStream << QObject::tr("Unsupported format %1").arg(format) << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
