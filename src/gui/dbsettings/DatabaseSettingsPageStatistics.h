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

#ifndef KEEPASSXC_DATABASESETTINGSPAGESTATISTICS_H
#define KEEPASSXC_DATABASESETTINGSPAGESTATISTICS_H

#include <QWidget>

#include "DatabaseSettingsDialog.h"

class DatabaseSettingsPageStatistics : public IDatabaseSettingsPage
{
public:
    QString name() override;
    QIcon icon() override;
    QWidget* createWidget() override;
    void loadSettings(QWidget* widget, QSharedPointer<Database> db) override;
    void saveSettings(QWidget* widget) override;
};

#endif // KEEPASSXC_DATABASESETTINGSPAGESTATISTICS_H
