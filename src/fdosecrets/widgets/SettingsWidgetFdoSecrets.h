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

#ifndef KEEPASSXC_SETTINGSWIDGETFDOSECRETS_H
#define KEEPASSXC_SETTINGSWIDGETFDOSECRETS_H

#include <QScopedPointer>
#include <QWidget>

class QAbstractItemView;
class QItemEditorCreatorBase;
class QItemEditorFactory;

namespace FdoSecrets
{

    class Session;
    class Collection;

} // namespace FdoSecrets

class FdoSecretsPlugin;

namespace Ui
{
    class SettingsWidgetFdoSecrets;
}
class SettingsWidgetFdoSecrets : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidgetFdoSecrets(FdoSecretsPlugin* plugin, QWidget* parent = nullptr);
    ~SettingsWidgetFdoSecrets() override;

public slots:
    void loadSettings();
    void saveSettings();

private:
    void setupView(QAbstractItemView* view, int manageColumn, int editorTypeId, QItemEditorCreatorBase* creator);

private:
    QScopedPointer<Ui::SettingsWidgetFdoSecrets> m_ui;
    QScopedPointer<QItemEditorFactory> m_factory;
    FdoSecretsPlugin* m_plugin;
};

#endif // KEEPASSXC_SETTINGSWIDGETFDOSECRETS_H
