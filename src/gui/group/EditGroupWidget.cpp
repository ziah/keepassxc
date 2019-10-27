/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "EditGroupWidget.h"
#include "gui/Font.h"
#include "ui_EditGroupWidgetMain.h"

#include "core/Config.h"
#include "core/FilePath.h"
#include "core/Metadata.h"
#include "gui/EditWidgetIcons.h"
#include "gui/EditWidgetProperties.h"
#include "gui/MessageBox.h"

#if defined(WITH_XC_KEESHARE)
#include "keeshare/group/EditGroupPageKeeShare.h"
#endif

class EditGroupWidget::ExtraPage
{
public:
    ExtraPage(IEditGroupPage* page, QWidget* widget)
        : editPage(page)
        , widget(widget)
    {
    }

    void set(Group* temporaryGroup, QSharedPointer<Database> database) const
    {
        editPage->set(widget, temporaryGroup, database);
    }

    void assign() const
    {
        editPage->assign(widget);
    }

    QWidget* getWidget()
    {
        return widget;
    }

private:
    QSharedPointer<IEditGroupPage> editPage;
    QWidget* widget;
};

EditGroupWidget::EditGroupWidget(QWidget* parent)
    : EditWidget(parent)
    , m_mainUi(new Ui::EditGroupWidgetMain())
    , m_editGroupWidgetMain(new QWidget())
    , m_editGroupWidgetIcons(new EditWidgetIcons())
    , m_editWidgetProperties(new EditWidgetProperties())
    , m_group(nullptr)
{
    m_mainUi->setupUi(m_editGroupWidgetMain);

    addPage(tr("Group"), FilePath::instance()->icon("actions", "document-edit"), m_editGroupWidgetMain);
    addPage(tr("Icon"), FilePath::instance()->icon("apps", "preferences-desktop-icons"), m_editGroupWidgetIcons);
#if defined(WITH_XC_KEESHARE)
    addEditPage(new EditGroupPageKeeShare(this));
#endif
    addPage(tr("Properties"), FilePath::instance()->icon("actions", "document-properties"), m_editWidgetProperties);

    connect(m_mainUi->expireCheck, SIGNAL(toggled(bool)), m_mainUi->expireDatePicker, SLOT(setEnabled(bool)));
    connect(m_mainUi->autoTypeSequenceCustomRadio,
            SIGNAL(toggled(bool)),
            m_mainUi->autoTypeSequenceCustomEdit,
            SLOT(setEnabled(bool)));

    connect(this, SIGNAL(apply()), SLOT(apply()));
    connect(this, SIGNAL(accepted()), SLOT(save()));
    connect(this, SIGNAL(rejected()), SLOT(cancel()));

    // clang-format off
    connect(m_editGroupWidgetIcons,
            SIGNAL(messageEditEntry(QString,MessageWidget::MessageType)),
            SLOT(showMessage(QString,MessageWidget::MessageType)));
    // clang-format on

    connect(m_editGroupWidgetIcons, SIGNAL(messageEditEntryDismiss()), SLOT(hideMessage()));

    setupModifiedTracking();
}

EditGroupWidget::~EditGroupWidget()
{
}

void EditGroupWidget::setupModifiedTracking()
{
    // Group tab
    connect(m_mainUi->editName, SIGNAL(textChanged(QString)), SLOT(setModified()));
    connect(m_mainUi->editNotes, SIGNAL(textChanged()), SLOT(setModified()));
    connect(m_mainUi->expireCheck, SIGNAL(stateChanged(int)), SLOT(setModified()));
    connect(m_mainUi->expireDatePicker, SIGNAL(dateTimeChanged(QDateTime)), SLOT(setModified()));
    connect(m_mainUi->searchComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setModified()));
    connect(m_mainUi->autotypeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setModified()));
    connect(m_mainUi->autoTypeSequenceInherit, SIGNAL(toggled(bool)), SLOT(setModified()));
    connect(m_mainUi->autoTypeSequenceCustomRadio, SIGNAL(toggled(bool)), SLOT(setModified()));
    connect(m_mainUi->autoTypeSequenceCustomEdit, SIGNAL(textChanged(QString)), SLOT(setModified()));

    // Icon tab
    connect(m_editGroupWidgetIcons, SIGNAL(widgetUpdated()), SLOT(setModified()));
}

void EditGroupWidget::loadGroup(Group* group, bool create, const QSharedPointer<Database>& database)
{
    m_group = group;
    m_db = database;

    m_temporaryGroup.reset(group->clone(Entry::CloneNoFlags, Group::CloneNoFlags));
    connect(m_temporaryGroup->customData(), SIGNAL(customDataModified()), SLOT(setModified()));

    if (create) {
        setHeadline(tr("Add group"));
    } else {
        setHeadline(tr("Edit group"));
    }

    if (m_group->parentGroup()) {
        addTriStateItems(m_mainUi->searchComboBox, m_group->parentGroup()->resolveSearchingEnabled());
        addTriStateItems(m_mainUi->autotypeComboBox, m_group->parentGroup()->resolveAutoTypeEnabled());
    } else {
        addTriStateItems(m_mainUi->searchComboBox, true);
        addTriStateItems(m_mainUi->autotypeComboBox, true);
    }

    m_mainUi->editName->setText(m_group->name());
    m_mainUi->editNotes->setPlainText(m_group->notes());
    m_mainUi->expireCheck->setChecked(group->timeInfo().expires());
    m_mainUi->expireDatePicker->setDateTime(group->timeInfo().expiryTime().toLocalTime());
    m_mainUi->searchComboBox->setCurrentIndex(indexFromTriState(group->searchingEnabled()));
    m_mainUi->autotypeComboBox->setCurrentIndex(indexFromTriState(group->autoTypeEnabled()));
    if (group->defaultAutoTypeSequence().isEmpty()) {
        m_mainUi->autoTypeSequenceInherit->setChecked(true);
    } else {
        m_mainUi->autoTypeSequenceCustomRadio->setChecked(true);
    }
    m_mainUi->autoTypeSequenceCustomEdit->setText(group->effectiveAutoTypeSequence());

    if (config()->get("GUI/MonospaceNotes", false).toBool()) {
        m_mainUi->editNotes->setFont(Font::fixedFont());
    } else {
        m_mainUi->editNotes->setFont(Font::defaultFont());
    }

    IconStruct iconStruct;
    iconStruct.uuid = m_temporaryGroup->iconUuid();
    iconStruct.number = m_temporaryGroup->iconNumber();
    m_editGroupWidgetIcons->load(m_temporaryGroup->uuid(), m_db, iconStruct);
    m_editWidgetProperties->setFields(m_temporaryGroup->timeInfo(), m_temporaryGroup->uuid());
    m_editWidgetProperties->setCustomData(m_temporaryGroup->customData());

    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.set(m_temporaryGroup.data(), m_db);
    }

    setCurrentPage(0);

    m_mainUi->editName->setFocus();

    // Force the user to Save/Discard new groups
    showApplyButton(!create);

    setModified(false);
}

void EditGroupWidget::save()
{
    apply();
    clear();
    emit editFinished(true);
}

void EditGroupWidget::apply()
{
    m_temporaryGroup->setName(m_mainUi->editName->text());
    m_temporaryGroup->setNotes(m_mainUi->editNotes->toPlainText());
    m_temporaryGroup->setExpires(m_mainUi->expireCheck->isChecked());
    m_temporaryGroup->setExpiryTime(m_mainUi->expireDatePicker->dateTime().toUTC());

    m_temporaryGroup->setSearchingEnabled(triStateFromIndex(m_mainUi->searchComboBox->currentIndex()));
    m_temporaryGroup->setAutoTypeEnabled(triStateFromIndex(m_mainUi->autotypeComboBox->currentIndex()));

    if (m_mainUi->autoTypeSequenceInherit->isChecked()) {
        m_temporaryGroup->setDefaultAutoTypeSequence(QString());
    } else {
        m_temporaryGroup->setDefaultAutoTypeSequence(m_mainUi->autoTypeSequenceCustomEdit->text());
    }

    IconStruct iconStruct = m_editGroupWidgetIcons->state();

    if (iconStruct.number < 0) {
        m_temporaryGroup->setIcon(Group::DefaultIconNumber);
    } else if (iconStruct.uuid.isNull()) {
        m_temporaryGroup->setIcon(iconStruct.number);
    } else {
        m_temporaryGroup->setIcon(iconStruct.uuid);
    }

    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.assign();
    }

    // Icons add/remove are applied globally outside the transaction!
    m_group->copyDataFrom(m_temporaryGroup.data());

    // Assign the icon to children if selected
    if (iconStruct.applyTo == ApplyIconToOptions::CHILD_GROUPS
        || iconStruct.applyTo == ApplyIconToOptions::ALL_CHILDREN) {
        m_group->applyGroupIconToChildGroups();
    }

    if (iconStruct.applyTo == ApplyIconToOptions::CHILD_ENTRIES
        || iconStruct.applyTo == ApplyIconToOptions::ALL_CHILDREN) {
        m_group->applyGroupIconToChildEntries();
    }

    setModified(false);
}

void EditGroupWidget::cancel()
{
    if (!m_group->iconUuid().isNull() && !m_db->metadata()->containsCustomIcon(m_group->iconUuid())) {
        m_group->setIcon(Entry::DefaultIconNumber);
    }

    if (isModified()) {
        auto result = MessageBox::question(this,
                                           QString(),
                                           tr("Entry has unsaved changes"),
                                           MessageBox::Cancel | MessageBox::Save | MessageBox::Discard,
                                           MessageBox::Cancel);
        if (result == MessageBox::Cancel) {
            return;
        }
        if (result == MessageBox::Save) {
            apply();
            setModified(false);
        }
    }

    clear();
    emit editFinished(false);
}

void EditGroupWidget::clear()
{
    m_group = nullptr;
    m_db.reset();
    m_temporaryGroup.reset(nullptr);
    m_editGroupWidgetIcons->reset();
}

void EditGroupWidget::addEditPage(IEditGroupPage* page)
{
    QWidget* widget = page->createWidget();
    widget->setParent(this);

    m_extraPages.append(ExtraPage(page, widget));
    addPage(page->name(), page->icon(), widget);
}

void EditGroupWidget::addTriStateItems(QComboBox* comboBox, bool inheritDefault)
{
    QString inheritDefaultString;
    if (inheritDefault) {
        inheritDefaultString = tr("Enable");
    } else {
        inheritDefaultString = tr("Disable");
    }

    comboBox->clear();
    comboBox->addItem(tr("Inherit from parent group (%1)").arg(inheritDefaultString));
    comboBox->addItem(tr("Enable"));
    comboBox->addItem(tr("Disable"));
}

int EditGroupWidget::indexFromTriState(Group::TriState triState)
{
    switch (triState) {
    case Group::Inherit:
        return 0;
    case Group::Enable:
        return 1;
    case Group::Disable:
        return 2;
    default:
        Q_ASSERT(false);
        return 0;
    }
}

Group::TriState EditGroupWidget::triStateFromIndex(int index)
{
    switch (index) {
    case 0:
        return Group::Inherit;
    case 1:
        return Group::Enable;
    case 2:
        return Group::Disable;
    default:
        Q_ASSERT(false);
        return Group::Inherit;
    }
}
