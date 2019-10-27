/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "ApplicationSettingsWidget.h"
#include "ui_ApplicationSettingsWidgetGeneral.h"
#include "ui_ApplicationSettingsWidgetSecurity.h"

#include "config-keepassx.h"

#include "autotype/AutoType.h"
#include "core/Config.h"
#include "core/FilePath.h"
#include "core/Global.h"
#include "core/Translator.h"

#include "MessageBox.h"
#include "touchid/TouchID.h"

class ApplicationSettingsWidget::ExtraPage
{
public:
    ExtraPage(ISettingsPage* page, QWidget* widget)
        : settingsPage(page)
        , widget(widget)
    {
    }

    void loadSettings() const
    {
        settingsPage->loadSettings(widget);
    }

    void saveSettings() const
    {
        settingsPage->saveSettings(widget);
    }

private:
    QSharedPointer<ISettingsPage> settingsPage;
    QWidget* widget;
};

/**
 * Helper class to ignore mouse wheel events on non-focused widgets
 * NOTE: The widget must NOT have a focus policy of "WHEEL"
 */
class MouseWheelEventFilter : public QObject
{
public:
    explicit MouseWheelEventFilter(QObject* parent)
        : QObject(parent){};

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        const auto* widget = qobject_cast<QWidget*>(obj);
        if (event->type() == QEvent::Wheel && widget && !widget->hasFocus()) {
            event->ignore();
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
};

ApplicationSettingsWidget::ApplicationSettingsWidget(QWidget* parent)
    : EditWidget(parent)
    , m_secWidget(new QWidget())
    , m_generalWidget(new QWidget())
    , m_secUi(new Ui::ApplicationSettingsWidgetSecurity())
    , m_generalUi(new Ui::ApplicationSettingsWidgetGeneral())
    , m_globalAutoTypeKey(static_cast<Qt::Key>(0))
    , m_globalAutoTypeModifiers(Qt::NoModifier)
{
    setHeadline(tr("Application Settings"));
    showApplyButton(false);

    m_secUi->setupUi(m_secWidget);
    m_generalUi->setupUi(m_generalWidget);
    addPage(tr("General"), FilePath::instance()->icon("categories", "preferences-other"), m_generalWidget);
    addPage(tr("Security"), FilePath::instance()->icon("status", "security-high"), m_secWidget);

    if (!autoType()->isAvailable()) {
        m_generalUi->generalSettingsTabWidget->removeTab(1);
    }

    connect(this, SIGNAL(accepted()), SLOT(saveSettings()));
    connect(this, SIGNAL(rejected()), SLOT(reject()));

    // clang-format off
    connect(m_generalUi->autoSaveAfterEveryChangeCheckBox, SIGNAL(toggled(bool)), SLOT(autoSaveToggled(bool)));
    connect(m_generalUi->hideWindowOnCopyCheckBox, SIGNAL(toggled(bool)), SLOT(hideWindowOnCopyCheckBoxToggled(bool)));
    connect(m_generalUi->systrayShowCheckBox, SIGNAL(toggled(bool)), SLOT(systrayToggled(bool)));
    connect(m_generalUi->toolbarHideCheckBox, SIGNAL(toggled(bool)), SLOT(toolbarSettingsToggled(bool)));
    connect(m_generalUi->rememberLastDatabasesCheckBox, SIGNAL(toggled(bool)), SLOT(rememberDatabasesToggled(bool)));
    connect(m_generalUi->resetSettingsButton, SIGNAL(clicked()), SLOT(resetSettings()));

    connect(m_secUi->clearClipboardCheckBox, SIGNAL(toggled(bool)),
            m_secUi->clearClipboardSpinBox, SLOT(setEnabled(bool)));
    connect(m_secUi->clearSearchCheckBox, SIGNAL(toggled(bool)),
            m_secUi->clearSearchSpinBox, SLOT(setEnabled(bool)));
    connect(m_secUi->lockDatabaseIdleCheckBox, SIGNAL(toggled(bool)),
            m_secUi->lockDatabaseIdleSpinBox, SLOT(setEnabled(bool)));
    connect(m_secUi->touchIDResetCheckBox, SIGNAL(toggled(bool)),
            m_secUi->touchIDResetSpinBox, SLOT(setEnabled(bool)));
    // clang-format on

    // Disable mouse wheel grab when scrolling
    // This prevents combo box and spinner values from changing without explicit focus
    auto mouseWheelFilter = new MouseWheelEventFilter(this);
    m_generalUi->faviconTimeoutSpinBox->installEventFilter(mouseWheelFilter);
    m_generalUi->toolButtonStyleComboBox->installEventFilter(mouseWheelFilter);
    m_generalUi->languageComboBox->installEventFilter(mouseWheelFilter);

#ifdef WITH_XC_UPDATECHECK
    connect(m_generalUi->checkForUpdatesOnStartupCheckBox, SIGNAL(toggled(bool)), SLOT(checkUpdatesToggled(bool)));
#else
    m_generalUi->checkForUpdatesOnStartupCheckBox->setVisible(false);
    m_generalUi->checkForUpdatesIncludeBetasCheckBox->setVisible(false);
    m_generalUi->checkUpdatesSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
#endif

#ifndef WITH_XC_NETWORKING
    m_secUi->privacy->setVisible(false);
    m_generalUi->faviconTimeoutLabel->setVisible(false);
    m_generalUi->faviconTimeoutSpinBox->setVisible(false);
#endif

#ifndef WITH_XC_TOUCHID
    bool hideTouchID = true;
#else
    bool hideTouchID = !TouchID::getInstance().isAvailable();
#endif
    if (hideTouchID) {
        m_secUi->touchIDResetCheckBox->setVisible(false);
        m_secUi->touchIDResetSpinBox->setVisible(false);
        m_secUi->touchIDResetOnScreenLockCheckBox->setVisible(false);
    }
}

ApplicationSettingsWidget::~ApplicationSettingsWidget()
{
}

void ApplicationSettingsWidget::addSettingsPage(ISettingsPage* page)
{
    QWidget* widget = page->createWidget();
    widget->setParent(this);
    m_extraPages.append(ExtraPage(page, widget));
    addPage(page->name(), page->icon(), widget);
}

void ApplicationSettingsWidget::loadSettings()
{
    if (config()->hasAccessError()) {
        showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
    }

#ifdef QT_DEBUG
    m_generalUi->singleInstanceCheckBox->setEnabled(false);
#endif
    m_generalUi->singleInstanceCheckBox->setChecked(config()->get("SingleInstance").toBool());
    m_generalUi->rememberLastDatabasesCheckBox->setChecked(config()->get("RememberLastDatabases").toBool());
    m_generalUi->rememberLastKeyFilesCheckBox->setChecked(config()->get("RememberLastKeyFiles").toBool());
    m_generalUi->openPreviousDatabasesOnStartupCheckBox->setChecked(
        config()->get("OpenPreviousDatabasesOnStartup").toBool());
    m_generalUi->autoSaveAfterEveryChangeCheckBox->setChecked(config()->get("AutoSaveAfterEveryChange").toBool());
    m_generalUi->autoSaveOnExitCheckBox->setChecked(config()->get("AutoSaveOnExit").toBool());
    m_generalUi->backupBeforeSaveCheckBox->setChecked(config()->get("BackupBeforeSave").toBool());
    m_generalUi->useAtomicSavesCheckBox->setChecked(config()->get("UseAtomicSaves").toBool());
    m_generalUi->autoReloadOnChangeCheckBox->setChecked(config()->get("AutoReloadOnChange").toBool());
    m_generalUi->minimizeAfterUnlockCheckBox->setChecked(config()->get("MinimizeAfterUnlock").toBool());
    m_generalUi->minimizeOnOpenUrlCheckBox->setChecked(config()->get("MinimizeOnOpenUrl").toBool());
    m_generalUi->hideWindowOnCopyCheckBox->setChecked(config()->get("HideWindowOnCopy").toBool());
    m_generalUi->minimizeOnCopyRadioButton->setChecked(config()->get("MinimizeOnCopy").toBool());
    m_generalUi->dropToBackgroundOnCopyRadioButton->setChecked(config()->get("DropToBackgroundOnCopy").toBool());
    m_generalUi->useGroupIconOnEntryCreationCheckBox->setChecked(config()->get("UseGroupIconOnEntryCreation").toBool());
    m_generalUi->autoTypeEntryTitleMatchCheckBox->setChecked(config()->get("AutoTypeEntryTitleMatch").toBool());
    m_generalUi->autoTypeEntryURLMatchCheckBox->setChecked(config()->get("AutoTypeEntryURLMatch").toBool());
    m_generalUi->ignoreGroupExpansionCheckBox->setChecked(config()->get("IgnoreGroupExpansion").toBool());
    m_generalUi->faviconTimeoutSpinBox->setValue(config()->get("FaviconDownloadTimeout").toInt());

    if (!m_generalUi->hideWindowOnCopyCheckBox->isChecked()) {
        hideWindowOnCopyCheckBoxToggled(false);
    }

    m_generalUi->languageComboBox->clear();
    QList<QPair<QString, QString>> languages = Translator::availableLanguages();
    for (const auto& language : languages) {
        m_generalUi->languageComboBox->addItem(language.second, language.first);
    }
    int defaultIndex = m_generalUi->languageComboBox->findData(config()->get("GUI/Language"));
    if (defaultIndex > 0) {
        m_generalUi->languageComboBox->setCurrentIndex(defaultIndex);
    }

    m_generalUi->previewHideCheckBox->setChecked(config()->get("GUI/HidePreviewPanel").toBool());
    m_generalUi->toolbarHideCheckBox->setChecked(config()->get("GUI/HideToolbar").toBool());
    m_generalUi->toolbarMovableCheckBox->setChecked(config()->get("GUI/MovableToolbar").toBool());
    m_generalUi->monospaceNotesCheckBox->setChecked(config()->get("GUI/MonospaceNotes").toBool());

    m_generalUi->toolButtonStyleComboBox->clear();
    m_generalUi->toolButtonStyleComboBox->addItem(tr("Icon only"), Qt::ToolButtonIconOnly);
    m_generalUi->toolButtonStyleComboBox->addItem(tr("Text only"), Qt::ToolButtonTextOnly);
    m_generalUi->toolButtonStyleComboBox->addItem(tr("Text beside icon"), Qt::ToolButtonTextBesideIcon);
    m_generalUi->toolButtonStyleComboBox->addItem(tr("Text under icon"), Qt::ToolButtonTextUnderIcon);
    m_generalUi->toolButtonStyleComboBox->addItem(tr("Follow style"), Qt::ToolButtonFollowStyle);
    int toolButtonStyleIndex = m_generalUi->toolButtonStyleComboBox->findData(config()->get("GUI/ToolButtonStyle"));
    if (toolButtonStyleIndex > 0) {
        m_generalUi->toolButtonStyleComboBox->setCurrentIndex(toolButtonStyleIndex);
    }

    m_generalUi->systrayShowCheckBox->setChecked(config()->get("GUI/ShowTrayIcon").toBool());
    m_generalUi->systrayDarkIconCheckBox->setChecked(config()->get("GUI/DarkTrayIcon").toBool());
    m_generalUi->systrayMinimizeToTrayCheckBox->setChecked(config()->get("GUI/MinimizeToTray").toBool());
    m_generalUi->minimizeOnCloseCheckBox->setChecked(config()->get("GUI/MinimizeOnClose").toBool());
    m_generalUi->systrayMinimizeOnStartup->setChecked(config()->get("GUI/MinimizeOnStartup").toBool());
    m_generalUi->checkForUpdatesOnStartupCheckBox->setChecked(config()->get("GUI/CheckForUpdates").toBool());
    m_generalUi->checkForUpdatesIncludeBetasCheckBox->setChecked(
        config()->get("GUI/CheckForUpdatesIncludeBetas").toBool());
    m_generalUi->autoTypeAskCheckBox->setChecked(config()->get("security/autotypeask").toBool());

    if (autoType()->isAvailable()) {
        m_globalAutoTypeKey = static_cast<Qt::Key>(config()->get("GlobalAutoTypeKey").toInt());
        m_globalAutoTypeModifiers =
            static_cast<Qt::KeyboardModifiers>(config()->get("GlobalAutoTypeModifiers").toInt());
        if (m_globalAutoTypeKey > 0 && m_globalAutoTypeModifiers > 0) {
            m_generalUi->autoTypeShortcutWidget->setShortcut(m_globalAutoTypeKey, m_globalAutoTypeModifiers);
        }
        m_generalUi->autoTypeShortcutWidget->setAttribute(Qt::WA_MacShowFocusRect, true);
        m_generalUi->autoTypeDelaySpinBox->setValue(config()->get("AutoTypeDelay").toInt());
        m_generalUi->autoTypeStartDelaySpinBox->setValue(config()->get("AutoTypeStartDelay").toInt());
    }

    m_secUi->clearClipboardCheckBox->setChecked(config()->get("security/clearclipboard").toBool());
    m_secUi->clearClipboardSpinBox->setValue(config()->get("security/clearclipboardtimeout").toInt());

    m_secUi->clearSearchCheckBox->setChecked(config()->get("security/clearsearch").toBool());
    m_secUi->clearSearchSpinBox->setValue(config()->get("security/clearsearchtimeout").toInt());

    m_secUi->lockDatabaseIdleCheckBox->setChecked(config()->get("security/lockdatabaseidle").toBool());
    m_secUi->lockDatabaseIdleSpinBox->setValue(config()->get("security/lockdatabaseidlesec").toInt());
    m_secUi->lockDatabaseMinimizeCheckBox->setChecked(config()->get("security/lockdatabaseminimize").toBool());
    m_secUi->lockDatabaseOnScreenLockCheckBox->setChecked(config()->get("security/lockdatabasescreenlock").toBool());
    m_secUi->relockDatabaseAutoTypeCheckBox->setChecked(config()->get("security/relockautotype").toBool());
    m_secUi->fallbackToSearch->setChecked(config()->get("security/IconDownloadFallback").toBool());

    m_secUi->passwordCleartextCheckBox->setChecked(config()->get("security/passwordscleartext").toBool());
    m_secUi->passwordShowDotsCheckBox->setChecked(config()->get("security/passwordemptynodots").toBool());
    m_secUi->passwordPreviewCleartextCheckBox->setChecked(config()->get("security/HidePasswordPreviewPanel").toBool());
    m_secUi->passwordRepeatCheckBox->setChecked(config()->get("security/passwordsrepeat").toBool());
    m_secUi->hideNotesCheckBox->setChecked(config()->get("security/hidenotes").toBool());

    m_secUi->touchIDResetCheckBox->setChecked(config()->get("security/resettouchid").toBool());
    m_secUi->touchIDResetSpinBox->setValue(config()->get("security/resettouchidtimeout").toInt());
    m_secUi->touchIDResetOnScreenLockCheckBox->setChecked(config()->get("security/resettouchidscreenlock").toBool());

    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.loadSettings();
    }

    setCurrentPage(0);
}

void ApplicationSettingsWidget::saveSettings()
{
    if (config()->hasAccessError()) {
        showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
        // We prevent closing the settings page if we could not write to
        // the config file.
        return;
    }

    config()->set("SingleInstance", m_generalUi->singleInstanceCheckBox->isChecked());
    config()->set("RememberLastDatabases", m_generalUi->rememberLastDatabasesCheckBox->isChecked());
    config()->set("RememberLastKeyFiles", m_generalUi->rememberLastKeyFilesCheckBox->isChecked());
    config()->set("OpenPreviousDatabasesOnStartup", m_generalUi->openPreviousDatabasesOnStartupCheckBox->isChecked());
    config()->set("AutoSaveAfterEveryChange", m_generalUi->autoSaveAfterEveryChangeCheckBox->isChecked());
    config()->set("AutoSaveOnExit", m_generalUi->autoSaveOnExitCheckBox->isChecked());
    config()->set("BackupBeforeSave", m_generalUi->backupBeforeSaveCheckBox->isChecked());
    config()->set("UseAtomicSaves", m_generalUi->useAtomicSavesCheckBox->isChecked());
    config()->set("AutoReloadOnChange", m_generalUi->autoReloadOnChangeCheckBox->isChecked());
    config()->set("MinimizeAfterUnlock", m_generalUi->minimizeAfterUnlockCheckBox->isChecked());
    config()->set("MinimizeOnOpenUrl", m_generalUi->minimizeOnOpenUrlCheckBox->isChecked());
    config()->set("HideWindowOnCopy", m_generalUi->hideWindowOnCopyCheckBox->isChecked());
    config()->set("MinimizeOnCopy", m_generalUi->minimizeOnCopyRadioButton->isChecked());
    config()->set("DropToBackgroundOnCopy", m_generalUi->dropToBackgroundOnCopyRadioButton->isChecked());
    config()->set("UseGroupIconOnEntryCreation", m_generalUi->useGroupIconOnEntryCreationCheckBox->isChecked());
    config()->set("IgnoreGroupExpansion", m_generalUi->ignoreGroupExpansionCheckBox->isChecked());
    config()->set("AutoTypeEntryTitleMatch", m_generalUi->autoTypeEntryTitleMatchCheckBox->isChecked());
    config()->set("AutoTypeEntryURLMatch", m_generalUi->autoTypeEntryURLMatchCheckBox->isChecked());
    int currentLangIndex = m_generalUi->languageComboBox->currentIndex();
    config()->set("FaviconDownloadTimeout", m_generalUi->faviconTimeoutSpinBox->value());

    config()->set("GUI/Language", m_generalUi->languageComboBox->itemData(currentLangIndex).toString());

    config()->set("GUI/HidePreviewPanel", m_generalUi->previewHideCheckBox->isChecked());
    config()->set("GUI/HideToolbar", m_generalUi->toolbarHideCheckBox->isChecked());
    config()->set("GUI/MovableToolbar", m_generalUi->toolbarMovableCheckBox->isChecked());
    config()->set("GUI/MonospaceNotes", m_generalUi->monospaceNotesCheckBox->isChecked());

    int currentToolButtonStyleIndex = m_generalUi->toolButtonStyleComboBox->currentIndex();
    config()->set("GUI/ToolButtonStyle",
                  m_generalUi->toolButtonStyleComboBox->itemData(currentToolButtonStyleIndex).toString());

    config()->set("GUI/ShowTrayIcon", m_generalUi->systrayShowCheckBox->isChecked());
    config()->set("GUI/DarkTrayIcon", m_generalUi->systrayDarkIconCheckBox->isChecked());
    config()->set("GUI/MinimizeToTray", m_generalUi->systrayMinimizeToTrayCheckBox->isChecked());
    config()->set("GUI/MinimizeOnClose", m_generalUi->minimizeOnCloseCheckBox->isChecked());
    config()->set("GUI/MinimizeOnStartup", m_generalUi->systrayMinimizeOnStartup->isChecked());
    config()->set("GUI/CheckForUpdates", m_generalUi->checkForUpdatesOnStartupCheckBox->isChecked());
    config()->set("GUI/CheckForUpdatesIncludeBetas", m_generalUi->checkForUpdatesIncludeBetasCheckBox->isChecked());

    config()->set("security/autotypeask", m_generalUi->autoTypeAskCheckBox->isChecked());

    if (autoType()->isAvailable()) {
        config()->set("GlobalAutoTypeKey", m_generalUi->autoTypeShortcutWidget->key());
        config()->set("GlobalAutoTypeModifiers", static_cast<int>(m_generalUi->autoTypeShortcutWidget->modifiers()));
        config()->set("AutoTypeDelay", m_generalUi->autoTypeDelaySpinBox->value());
        config()->set("AutoTypeStartDelay", m_generalUi->autoTypeStartDelaySpinBox->value());
    }
    config()->set("security/clearclipboard", m_secUi->clearClipboardCheckBox->isChecked());
    config()->set("security/clearclipboardtimeout", m_secUi->clearClipboardSpinBox->value());

    config()->set("security/clearsearch", m_secUi->clearSearchCheckBox->isChecked());
    config()->set("security/clearsearchtimeout", m_secUi->clearSearchSpinBox->value());

    config()->set("security/lockdatabaseidle", m_secUi->lockDatabaseIdleCheckBox->isChecked());
    config()->set("security/lockdatabaseidlesec", m_secUi->lockDatabaseIdleSpinBox->value());
    config()->set("security/lockdatabaseminimize", m_secUi->lockDatabaseMinimizeCheckBox->isChecked());
    config()->set("security/lockdatabasescreenlock", m_secUi->lockDatabaseOnScreenLockCheckBox->isChecked());
    config()->set("security/relockautotype", m_secUi->relockDatabaseAutoTypeCheckBox->isChecked());
    config()->set("security/IconDownloadFallback", m_secUi->fallbackToSearch->isChecked());

    config()->set("security/passwordscleartext", m_secUi->passwordCleartextCheckBox->isChecked());
    config()->set("security/passwordemptynodots", m_secUi->passwordShowDotsCheckBox->isChecked());

    config()->set("security/HidePasswordPreviewPanel", m_secUi->passwordPreviewCleartextCheckBox->isChecked());
    config()->set("security/passwordsrepeat", m_secUi->passwordRepeatCheckBox->isChecked());
    config()->set("security/hidenotes", m_secUi->hideNotesCheckBox->isChecked());

    config()->set("security/resettouchid", m_secUi->touchIDResetCheckBox->isChecked());
    config()->set("security/resettouchidtimeout", m_secUi->touchIDResetSpinBox->value());
    config()->set("security/resettouchidscreenlock", m_secUi->touchIDResetOnScreenLockCheckBox->isChecked());

    // Security: clear storage if related settings are disabled
    if (!config()->get("RememberLastDatabases").toBool()) {
        config()->set("LastDatabases", {});
        config()->set("OpenPreviousDatabasesOnStartup", {});
        config()->set("LastActiveDatabase", {});
        config()->set("LastAttachmentDir", {});
    }

    if (!config()->get("RememberLastKeyFiles").toBool()) {
        config()->set("LastKeyFiles", {});
        config()->set("LastDir", "");
    }

    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.saveSettings();
    }
}

void ApplicationSettingsWidget::resetSettings()
{
    // Confirm reset
    auto ans = MessageBox::question(this,
                                    tr("Reset Settings?"),
                                    tr("Are you sure you want to reset all general and security settings to default?"),
                                    MessageBox::Reset | MessageBox::Cancel,
                                    MessageBox::Cancel);
    if (ans == MessageBox::Cancel) {
        return;
    }

    if (config()->hasAccessError()) {
        showMessage(tr("Access error for config file %1").arg(config()->getFileName()), MessageWidget::Error);
        // We prevent closing the settings page if we could not write to
        // the config file.
        return;
    }

    // Reset general and security settings to default
    config()->resetToDefaults();

    // Clear recently used data
    config()->set("LastDatabases", {});
    config()->set("OpenPreviousDatabasesOnStartup", {});
    config()->set("LastActiveDatabase", {});
    config()->set("LastAttachmentDir", {});
    config()->set("LastKeyFiles", {});
    config()->set("LastDir", "");

    // Save the Extra Pages (these are NOT reset)
    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.saveSettings();
    }

    config()->sync();

    // Refresh the settings widget and notify listeners
    loadSettings();
    emit settingsReset();
}

void ApplicationSettingsWidget::reject()
{
    // register the old key again as it might have changed
    if (m_globalAutoTypeKey > 0 && m_globalAutoTypeModifiers > 0) {
        autoType()->registerGlobalShortcut(m_globalAutoTypeKey, m_globalAutoTypeModifiers);
    }
}

void ApplicationSettingsWidget::autoSaveToggled(bool checked)
{
    // Explicitly enable auto-save on exit if it wasn't already
    if (checked && !m_generalUi->autoSaveOnExitCheckBox->isChecked()) {
        m_generalUi->autoSaveOnExitCheckBox->setChecked(true);
    }
    m_generalUi->autoSaveOnExitCheckBox->setEnabled(!checked);
}

void ApplicationSettingsWidget::hideWindowOnCopyCheckBoxToggled(bool checked)
{
    m_generalUi->minimizeOnCopyRadioButton->setEnabled(checked);
    m_generalUi->dropToBackgroundOnCopyRadioButton->setEnabled(checked);
}

void ApplicationSettingsWidget::systrayToggled(bool checked)
{
    m_generalUi->systrayDarkIconCheckBox->setEnabled(checked);
    m_generalUi->systrayMinimizeToTrayCheckBox->setEnabled(checked);
}

void ApplicationSettingsWidget::toolbarSettingsToggled(bool checked)
{
    m_generalUi->toolbarMovableCheckBox->setEnabled(!checked);
    m_generalUi->toolButtonStyleComboBox->setEnabled(!checked);
    m_generalUi->toolButtonStyleLabel->setEnabled(!checked);
}

void ApplicationSettingsWidget::rememberDatabasesToggled(bool checked)
{
    if (!checked) {
        m_generalUi->rememberLastKeyFilesCheckBox->setChecked(false);
        m_generalUi->openPreviousDatabasesOnStartupCheckBox->setChecked(false);
    }

    m_generalUi->rememberLastKeyFilesCheckBox->setEnabled(checked);
    m_generalUi->openPreviousDatabasesOnStartupCheckBox->setEnabled(checked);
}

void ApplicationSettingsWidget::checkUpdatesToggled(bool checked)
{
    m_generalUi->checkForUpdatesIncludeBetasCheckBox->setEnabled(checked);
}
