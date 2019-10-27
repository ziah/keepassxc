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

#ifndef KEEPASSXC_DATABASESETTINGSWIDGETSTATISTICS_H
#define KEEPASSXC_DATABASESETTINGSWIDGETSTATISTICS_H

#include <QIcon>
#include <QWidget>

class Database;
class QStandardItemModel;

namespace Ui
{
    class DatabaseSettingsWidgetStatistics;
}

class DatabaseSettingsWidgetStatistics : public QWidget
{
    Q_OBJECT
public:
    explicit DatabaseSettingsWidgetStatistics(QWidget* parent = nullptr);
    ~DatabaseSettingsWidgetStatistics();

    void loadSettings(QSharedPointer<Database> db);
    void saveSettings();

private:
    QScopedPointer<Ui::DatabaseSettingsWidgetStatistics> m_ui;

    QIcon m_errIcon;
    QScopedPointer<QStandardItemModel> m_referencesModel;

    void addStatsRow(QString name, QString value, bool bad = false, QString badMsg = "");
};

#endif // KEEPASSXC_DATABASESETTINGSWIDGETSTATISTICS_H
