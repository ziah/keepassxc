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

#include "EntryPreviewWidget.h"
#include "Font.h"
#include "ui_EntryPreviewWidget.h"

#include <QDesktopServices>
#include <QDir>

#include "core/Config.h"
#include "core/FilePath.h"
#include "entry/EntryAttachmentsModel.h"
#include "gui/Clipboard.h"
#if defined(WITH_XC_KEESHARE)
#include "keeshare/KeeShare.h"
#endif

namespace
{
    constexpr int GeneralTabIndex = 0;
}

EntryPreviewWidget::EntryPreviewWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EntryPreviewWidget())
    , m_locked(false)
    , m_currentEntry(nullptr)
    , m_currentGroup(nullptr)
    , m_selectedTabEntry(0)
    , m_selectedTabGroup(0)
{
    m_ui->setupUi(this);

    // Entry
    m_ui->entryTotpButton->setIcon(filePath()->icon("actions", "chronometer"));
    m_ui->entryCloseButton->setIcon(filePath()->icon("actions", "dialog-close"));
    m_ui->entryPasswordLabel->setFont(Font::fixedFont());
    m_ui->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    m_ui->toggleEntryNotesButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    m_ui->toggleGroupNotesButton->setIcon(filePath()->onOffIcon("actions", "password-show"));

    m_ui->entryAttachmentsWidget->setReadOnly(true);
    m_ui->entryAttachmentsWidget->setButtonsVisible(false);

    // Match background of read-only text edit fields with the window
    m_ui->entryPasswordLabel->setBackgroundRole(QPalette::Window);
    m_ui->entryUsernameLabel->setBackgroundRole(QPalette::Window);
    m_ui->entryNotesTextEdit->setBackgroundRole(QPalette::Window);
    m_ui->groupNotesTextEdit->setBackgroundRole(QPalette::Window);
    // Align notes text with label text
    m_ui->entryNotesTextEdit->document()->setDocumentMargin(0);
    m_ui->groupNotesTextEdit->document()->setDocumentMargin(0);

    connect(m_ui->entryUrlLabel, SIGNAL(linkActivated(QString)), SLOT(openEntryUrl()));

    connect(m_ui->entryTotpButton, SIGNAL(toggled(bool)), m_ui->entryTotpLabel, SLOT(setVisible(bool)));
    connect(m_ui->entryCloseButton, SIGNAL(clicked()), SLOT(hide()));
    connect(m_ui->togglePasswordButton, SIGNAL(clicked(bool)), SLOT(setPasswordVisible(bool)));
    connect(m_ui->toggleEntryNotesButton, SIGNAL(clicked(bool)), SLOT(setEntryNotesVisible(bool)));
    connect(m_ui->toggleGroupNotesButton, SIGNAL(clicked(bool)), SLOT(setGroupNotesVisible(bool)));
    connect(m_ui->entryTabWidget, SIGNAL(tabBarClicked(int)), SLOT(updateTabIndexes()), Qt::QueuedConnection);
    connect(&m_totpTimer, SIGNAL(timeout()), SLOT(updateTotpLabel()));

    // Group
    m_ui->groupCloseButton->setIcon(filePath()->icon("actions", "dialog-close"));
    connect(m_ui->groupCloseButton, SIGNAL(clicked()), SLOT(hide()));
    connect(m_ui->groupTabWidget, SIGNAL(tabBarClicked(int)), SLOT(updateTabIndexes()), Qt::QueuedConnection);

#if !defined(WITH_XC_KEESHARE)
    removeTab(m_ui->groupTabWidget, m_ui->groupShareTab);
#endif
}

EntryPreviewWidget::~EntryPreviewWidget()
{
}

void EntryPreviewWidget::setEntry(Entry* selectedEntry)
{
    if (!selectedEntry) {
        hide();
        return;
    }

    m_currentEntry = selectedEntry;

    updateEntryHeaderLine();
    updateEntryTotp();
    updateEntryGeneralTab();
    updateEntryAdvancedTab();
    updateEntryAutotypeTab();

    setVisible(!config()->get("GUI/HidePreviewPanel").toBool());

    m_ui->stackedWidget->setCurrentWidget(m_ui->pageEntry);
    const int tabIndex = m_ui->entryTabWidget->isTabEnabled(m_selectedTabEntry) ? m_selectedTabEntry : GeneralTabIndex;
    Q_ASSERT(m_ui->entryTabWidget->isTabEnabled(GeneralTabIndex));
    m_ui->entryTabWidget->setCurrentIndex(tabIndex);
}

void EntryPreviewWidget::setGroup(Group* selectedGroup)
{
    if (!selectedGroup) {
        hide();
        return;
    }

    m_currentGroup = selectedGroup;
    updateGroupHeaderLine();
    updateGroupGeneralTab();

#if defined(WITH_XC_KEESHARE)
    updateGroupSharingTab();
#endif

    setVisible(!config()->get("GUI/HidePreviewPanel").toBool());

    m_ui->stackedWidget->setCurrentWidget(m_ui->pageGroup);
    const int tabIndex = m_ui->groupTabWidget->isTabEnabled(m_selectedTabGroup) ? m_selectedTabGroup : GeneralTabIndex;
    Q_ASSERT(m_ui->groupTabWidget->isTabEnabled(GeneralTabIndex));
    m_ui->groupTabWidget->setCurrentIndex(tabIndex);
}

void EntryPreviewWidget::setDatabaseMode(DatabaseWidget::Mode mode)
{
    m_locked = mode == DatabaseWidget::Mode::LockedMode;
    if (m_locked) {
        return;
    }

    if (mode == DatabaseWidget::Mode::ViewMode) {
        if (m_ui->stackedWidget->currentWidget() == m_ui->pageGroup) {
            setGroup(m_currentGroup);
        } else {
            setEntry(m_currentEntry);
        }
    }
}

void EntryPreviewWidget::updateEntryHeaderLine()
{
    Q_ASSERT(m_currentEntry);
    const QString title = m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->title());
    m_ui->entryTitleLabel->setRawText(hierarchy(m_currentEntry->group(), title));
    m_ui->entryIcon->setPixmap(preparePixmap(m_currentEntry->iconPixmap(), 16));
}

void EntryPreviewWidget::updateEntryTotp()
{
    Q_ASSERT(m_currentEntry);
    const bool hasTotp = m_currentEntry->hasTotp();
    m_ui->entryTotpButton->setVisible(hasTotp);
    m_ui->entryTotpLabel->hide();
    m_ui->entryTotpButton->setChecked(false);

    if (hasTotp) {
        m_totpTimer.start(1000);
        updateTotpLabel();
    } else {
        m_ui->entryTotpLabel->clear();
        m_totpTimer.stop();
    }
}

void EntryPreviewWidget::setPasswordVisible(bool state)
{
    const QString password = m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->password());
    if (state) {
        m_ui->entryPasswordLabel->setText(password);
        m_ui->entryPasswordLabel->setCursorPosition(0);
    } else if (password.isEmpty() && config()->get("security/passwordemptynodots").toBool()) {
        m_ui->entryPasswordLabel->setText("");
    } else {
        m_ui->entryPasswordLabel->setText(QString("\u25cf").repeated(6));
    }
}

void EntryPreviewWidget::setEntryNotesVisible(bool state)
{
    setNotesVisible(m_ui->entryNotesTextEdit, m_currentEntry->notes(), state);
}

void EntryPreviewWidget::setGroupNotesVisible(bool state)
{
    setNotesVisible(m_ui->groupNotesTextEdit, m_currentGroup->notes(), state);
}

void EntryPreviewWidget::setNotesVisible(QTextEdit* notesWidget, const QString& notes, bool state)
{
    if (state) {
        notesWidget->setPlainText(notes);
        notesWidget->moveCursor(QTextCursor::Start);
        notesWidget->ensureCursorVisible();
    } else {
        if (!notes.isEmpty()) {
            notesWidget->setPlainText(QString("\u25cf").repeated(6));
        }
    }
}

void EntryPreviewWidget::updateEntryGeneralTab()
{
    Q_ASSERT(m_currentEntry);
    m_ui->entryUsernameLabel->setText(m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->username()));
    m_ui->entryUsernameLabel->setCursorPosition(0);

    if (config()->get("security/HidePasswordPreviewPanel").toBool()) {
        // Hide password
        setPasswordVisible(false);
        // Show the password toggle button if there are dots in the label
        m_ui->togglePasswordButton->setVisible(!m_ui->entryPasswordLabel->text().isEmpty());
        m_ui->togglePasswordButton->setChecked(false);
    } else {
        // Show password
        setPasswordVisible(true);
        m_ui->togglePasswordButton->setVisible(false);
    }

    if (config()->get("security/hidenotes").toBool()) {
        setEntryNotesVisible(false);
        m_ui->toggleEntryNotesButton->setVisible(!m_ui->entryNotesTextEdit->toPlainText().isEmpty());
        m_ui->toggleEntryNotesButton->setChecked(false);
    } else {
        setEntryNotesVisible(true);
        m_ui->toggleEntryNotesButton->setVisible(false);
    }

    if (config()->get("GUI/MonospaceNotes", false).toBool()) {
        m_ui->entryNotesTextEdit->setFont(Font::fixedFont());
    } else {
        m_ui->entryNotesTextEdit->setFont(Font::defaultFont());
    }

    m_ui->entryUrlLabel->setRawText(m_currentEntry->displayUrl());
    const QString url = m_currentEntry->url();
    if (!url.isEmpty()) {
        // URL is well formed and can be opened in a browser
        m_ui->entryUrlLabel->setUrl(m_currentEntry->resolveMultiplePlaceholders(url));
        m_ui->entryUrlLabel->setCursor(Qt::PointingHandCursor);
        m_ui->entryUrlLabel->setOpenExternalLinks(false);
    } else {
        m_ui->entryUrlLabel->setUrl({});
        m_ui->entryUrlLabel->setCursor(Qt::ArrowCursor);
    }

    const TimeInfo entryTime = m_currentEntry->timeInfo();
    const QString expires =
        entryTime.expires() ? entryTime.expiryTime().toLocalTime().toString(Qt::DefaultLocaleShortDate) : tr("Never");
    m_ui->entryExpirationLabel->setText(expires);
}

void EntryPreviewWidget::updateEntryAdvancedTab()
{
    Q_ASSERT(m_currentEntry);
    m_ui->entryAttributesEdit->clear();
    const EntryAttributes* attributes = m_currentEntry->attributes();
    const QStringList customAttributes = attributes->customKeys();
    const bool hasAttributes = !customAttributes.isEmpty();
    const bool hasAttachments = !m_currentEntry->attachments()->isEmpty();

    setTabEnabled(m_ui->entryTabWidget, m_ui->entryAdvancedTab, hasAttributes || hasAttachments);
    if (hasAttributes) {
        QString attributesText;
        for (const QString& key : customAttributes) {
            QString value = m_currentEntry->attributes()->value(key);
            if (m_currentEntry->attributes()->isProtected(key)) {
                value = "<i>" + tr("[PROTECTED]") + "</i>";
            }
            attributesText.append(tr("<b>%1</b>: %2", "attributes line").arg(key, value).append("<br/>"));
        }
        m_ui->entryAttributesEdit->setText(attributesText);
    }

    m_ui->entryAttachmentsWidget->setEntryAttachments(m_currentEntry->attachments());
}

void EntryPreviewWidget::updateEntryAutotypeTab()
{
    Q_ASSERT(m_currentEntry);
    m_ui->entryAutotypeTree->clear();
    QList<QTreeWidgetItem*> items;
    const AutoTypeAssociations* autotypeAssociations = m_currentEntry->autoTypeAssociations();
    const auto associations = autotypeAssociations->getAll();
    for (const auto& assoc : associations) {
        const QString sequence =
            assoc.sequence.isEmpty() ? m_currentEntry->effectiveAutoTypeSequence() : assoc.sequence;
        items.append(new QTreeWidgetItem(m_ui->entryAutotypeTree, {assoc.window, sequence}));
    }

    m_ui->entryAutotypeTree->addTopLevelItems(items);
    setTabEnabled(m_ui->entryTabWidget, m_ui->entryAutotypeTab, !items.isEmpty());
}

void EntryPreviewWidget::updateGroupHeaderLine()
{
    Q_ASSERT(m_currentGroup);
    m_ui->groupTitleLabel->setRawText(hierarchy(m_currentGroup, {}));
    m_ui->groupIcon->setPixmap(preparePixmap(m_currentGroup->iconPixmap(), 32));
}

void EntryPreviewWidget::updateGroupGeneralTab()
{
    Q_ASSERT(m_currentGroup);
    const QString searchingText = m_currentGroup->resolveSearchingEnabled() ? tr("Enabled") : tr("Disabled");
    m_ui->groupSearchingLabel->setText(searchingText);

    const QString autotypeText = m_currentGroup->resolveAutoTypeEnabled() ? tr("Enabled") : tr("Disabled");
    m_ui->groupAutotypeLabel->setText(autotypeText);

    const TimeInfo groupTime = m_currentGroup->timeInfo();
    const QString expiresText =
        groupTime.expires() ? groupTime.expiryTime().toString(Qt::DefaultLocaleShortDate) : tr("Never");
    m_ui->groupExpirationLabel->setText(expiresText);

    if (config()->get("security/hidenotes").toBool()) {
        setGroupNotesVisible(false);
        m_ui->toggleGroupNotesButton->setVisible(!m_ui->groupNotesTextEdit->toPlainText().isEmpty());
        m_ui->toggleGroupNotesButton->setChecked(false);
    } else {
        setGroupNotesVisible(true);
        m_ui->toggleGroupNotesButton->setVisible(false);
    }

    if (config()->get("GUI/MonospaceNotes", false).toBool()) {
        m_ui->groupNotesTextEdit->setFont(Font::fixedFont());
    } else {
        m_ui->groupNotesTextEdit->setFont(Font::defaultFont());
    }
}

#if defined(WITH_XC_KEESHARE)
void EntryPreviewWidget::updateGroupSharingTab()
{
    Q_ASSERT(m_currentGroup);
    setTabEnabled(m_ui->groupTabWidget, m_ui->groupShareTab, KeeShare::isShared(m_currentGroup));
    auto reference = KeeShare::referenceOf(m_currentGroup);
    m_ui->groupShareTypeLabel->setText(KeeShare::referenceTypeLabel(reference));
    m_ui->groupSharePathLabel->setText(reference.path);
}
#endif

void EntryPreviewWidget::updateTotpLabel()
{
    if (!m_locked && m_currentEntry && m_currentEntry->hasTotp()) {
        const QString totpCode = m_currentEntry->totp();
        const QString firstHalf = totpCode.left(totpCode.size() / 2);
        const QString secondHalf = totpCode.mid(totpCode.size() / 2);
        m_ui->entryTotpLabel->setText(firstHalf + " " + secondHalf);
    } else {
        m_ui->entryTotpLabel->clear();
        m_totpTimer.stop();
    }
}

void EntryPreviewWidget::updateTabIndexes()
{
    m_selectedTabEntry = m_ui->entryTabWidget->currentIndex();
    m_selectedTabGroup = m_ui->groupTabWidget->currentIndex();
}

void EntryPreviewWidget::openEntryUrl()
{
    if (m_currentEntry) {
        emit entryUrlActivated(m_currentEntry);
    }
}

void EntryPreviewWidget::removeTab(QTabWidget* tabWidget, QWidget* widget)
{
    const int tabIndex = tabWidget->indexOf(widget);
    Q_ASSERT(tabIndex != -1);
    tabWidget->removeTab(tabIndex);
}

void EntryPreviewWidget::setTabEnabled(QTabWidget* tabWidget, QWidget* widget, bool enabled)
{
    const int tabIndex = tabWidget->indexOf(widget);
    Q_ASSERT(tabIndex != -1);
    tabWidget->setTabEnabled(tabIndex, enabled);
}

QPixmap EntryPreviewWidget::preparePixmap(const QPixmap& pixmap, int size)
{
    if (pixmap.width() > size || pixmap.height() > size) {
        return pixmap.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return pixmap;
}

QString EntryPreviewWidget::hierarchy(const Group* group, const QString& title)
{
    const QString separator("] > [");
    QStringList hierarchy = group->hierarchy();
    QString groupList = QString("[%1]").arg(hierarchy.join(separator));

    return title.isEmpty() ? groupList : QString("%1 > %2").arg(groupList, title);
}
