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

#ifndef KEEPASSX_UNLOCKDATABASEDIALOG_H
#define KEEPASSX_UNLOCKDATABASEDIALOG_H

#include "core/Global.h"

#include <QDialog>
#include <QPointer>
#include <QSharedPointer>

class Database;
class DatabaseWidget;
class DatabaseOpenWidget;

class DatabaseOpenDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Intent
    {
        None,
        AutoType,
        Merge,
        Browser
    };

    explicit DatabaseOpenDialog(QWidget* parent = nullptr);
    void setFilePath(const QString& filePath);
    void setTargetDatabaseWidget(DatabaseWidget* dbWidget);
    void setIntent(Intent intent);
    Intent intent() const;
    QSharedPointer<Database> database();
    void clearForms();

signals:
    void dialogFinished(bool accepted, DatabaseWidget* dbWidget);

public slots:
    void complete(bool accepted);

private:
    QPointer<DatabaseOpenWidget> m_view;
    QSharedPointer<Database> m_db;
    QPointer<DatabaseWidget> m_dbWidget;
    Intent m_intent = Intent::None;
};

#endif // KEEPASSX_UNLOCKDATABASEDIALOG_H
