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

#ifndef TEST_OPVAULT_READER_H_
#define TEST_OPVAULT_READER_H_

#include <QMap>
#include <QObject>

class TestOpVaultReader : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testReadIntoDatabase();
    void testBandEntry1();
    void testKeyDerivation();

private:
    // absolute path to the .opvault directory
    QString m_opVaultPath;

    /*
     * Points to the file made by using the 1Password GUI to "Export all"
     * to its text file format, which are almost key=value pairs
     * except for multi-line strings.
     */
    QString m_opVaultTextExportPath;
    QString m_password;
    QMap<QString, QString> m_categoryMap;
};

#endif /* TEST_OPVAULT_READER_H_ */
