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

#ifndef KEEPASSXC_KEYCOMPONENTWIDGET_H
#define KEEPASSXC_KEYCOMPONENTWIDGET_H

#include <QPointer>
#include <QScopedPointer>
#include <QWidget>

namespace Ui
{
    class KeyComponentWidget;
}
class CompositeKey;
class QStackedWidget;

class KeyComponentWidget : public QWidget
{
    Q_OBJECT
    // clang-format off
    Q_PROPERTY(QString componentName READ m_componentName READ componentName
                   WRITE setComponentName NOTIFY nameChanged)
    Q_PROPERTY(QString componentDescription READ m_componentDescription READ componentDescription
                   WRITE setComponentDescription NOTIFY descriptionChanged)
    Q_PROPERTY(bool componentAdded READ m_isComponentAdded READ componentAdded
                   WRITE setComponentAdded NOTIFY componentAddChanged)
    // clang-format on

public:
    enum Page
    {
        AddNew = 0,
        Edit = 1,
        LeaveOrRemove = 2
    };

    explicit KeyComponentWidget(QWidget* parent = nullptr);
    explicit KeyComponentWidget(const QString& name, QWidget* parent = nullptr);
    Q_DISABLE_COPY(KeyComponentWidget);
    ~KeyComponentWidget() override;

    /**
     * Add the new key component to the given \link CompositeKey.
     * A caller should always check first with \link validate() if
     * the new key data is actually valid before adding it to a CompositeKey.
     *
     * @param key CompositeKey to add new key to
     * @return true if added successfully
     */
    virtual bool addToCompositeKey(QSharedPointer<CompositeKey> key) = 0;

    /**
     * Validate key component data to check if key component
     * may be added to a CompositeKey.
     *
     * @param errorMessage error message in case data is not valid
     * @return true if data is valid
     */
    virtual bool validate(QString& errorMessage) const = 0;

    void setComponentName(const QString& name);
    QString componentName() const;
    void setComponentDescription(const QString& name);
    QString componentDescription() const;
    void setComponentAdded(bool added);
    bool componentAdded() const;
    void changeVisiblePage(Page page);
    Page visiblePage() const;

protected:
    /**
     * Construct and return an instance of the key component edit widget
     * which is to be inserted into the component management UI.
     * Since previous widgets will be destroyed, every successive call to
     * this function must return a new widget.
     *
     * @return edit widget instance
     */
    virtual QWidget* componentEditWidget() = 0;

    /**
     * Initialize the key component widget created by \link componentEditWidget().
     * This method is called every time the component edit widget is shown.
     *
     * @param widget pointer to the widget
     */
    virtual void initComponentEditWidget(QWidget* widget) = 0;

signals:
    void nameChanged(const QString& newName);
    void descriptionChanged(const QString& newDescription);
    void componentAddChanged(bool added);
    void componentAddRequested();
    void componentEditRequested();
    void editCanceled();
    void componentRemovalRequested();

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void updateComponentName(const QString& name);
    void updateComponentDescription(const QString& decription);
    void updateAddStatus(bool added);
    void doAdd();
    void doEdit();
    void doRemove();
    void cancelEdit();
    void resetComponentEditWidget();
    void updateSize();

private:
    bool m_isComponentAdded = false;
    Page m_previousPage = Page::AddNew;
    QString m_componentName;
    QString m_componentDescription;
    QPointer<QWidget> m_componentWidget;

    const QScopedPointer<Ui::KeyComponentWidget> m_ui;
};

#endif // KEEPASSXC_KEYCOMPONENTWIDGET_H
