/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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
#include "Entry.h"

#include "config-keepassx.h"

#include "core/Clock.h"
#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "totp/totp.h"

#include <QDir>
#include <QRegularExpression>
#include <utility>

const int Entry::DefaultIconNumber = 0;
const int Entry::ResolveMaximumDepth = 10;
const QString Entry::AutoTypeSequenceUsername = "{USERNAME}{ENTER}";
const QString Entry::AutoTypeSequencePassword = "{PASSWORD}{ENTER}";

Entry::Entry()
    : m_attributes(new EntryAttributes(this))
    , m_attachments(new EntryAttachments(this))
    , m_autoTypeAssociations(new AutoTypeAssociations(this))
    , m_customData(new CustomData(this))
    , m_modifiedSinceBegin(false)
    , m_updateTimeinfo(true)
{
    m_data.iconNumber = DefaultIconNumber;
    m_data.autoTypeEnabled = true;
    m_data.autoTypeObfuscation = 0;

    connect(m_attributes, SIGNAL(entryAttributesModified()), SLOT(updateTotp()));
    connect(m_attributes, SIGNAL(entryAttributesModified()), this, SIGNAL(entryModified()));
    connect(m_attributes, SIGNAL(defaultKeyModified()), SLOT(emitDataChanged()));
    connect(m_attachments, SIGNAL(entryAttachmentsModified()), this, SIGNAL(entryModified()));
    connect(m_autoTypeAssociations, SIGNAL(modified()), SIGNAL(entryModified()));
    connect(m_customData, SIGNAL(customDataModified()), this, SIGNAL(entryModified()));

    connect(this, SIGNAL(entryModified()), SLOT(updateTimeinfo()));
    connect(this, SIGNAL(entryModified()), SLOT(updateModifiedSinceBegin()));
}

Entry::~Entry()
{
    setUpdateTimeinfo(false);
    if (m_group) {
        m_group->removeEntry(this);

        if (m_group->database()) {
            m_group->database()->addDeletedObject(m_uuid);
        }
    }

    qDeleteAll(m_history);
}

template <class T> inline bool Entry::set(T& property, const T& value)
{
    if (property != value) {
        property = value;
        emit entryModified();
        return true;
    }
    return false;
}

void Entry::updateTimeinfo()
{
    if (m_updateTimeinfo) {
        m_data.timeInfo.setLastModificationTime(Clock::currentDateTimeUtc());
        m_data.timeInfo.setLastAccessTime(Clock::currentDateTimeUtc());
    }
}

bool Entry::canUpdateTimeinfo() const
{
    return m_updateTimeinfo;
}

void Entry::setUpdateTimeinfo(bool value)
{
    m_updateTimeinfo = value;
}

QString Entry::buildReference(const QUuid& uuid, const QString& field)
{
    Q_ASSERT(EntryAttributes::DefaultAttributes.count(field) > 0);

    QString uuidStr = Tools::uuidToHex(uuid).toUpper();
    QString shortField;

    if (field == EntryAttributes::TitleKey) {
        shortField = "T";
    } else if (field == EntryAttributes::UserNameKey) {
        shortField = "U";
    } else if (field == EntryAttributes::PasswordKey) {
        shortField = "P";
    } else if (field == EntryAttributes::URLKey) {
        shortField = "A";
    } else if (field == EntryAttributes::NotesKey) {
        shortField = "N";
    }

    if (shortField.isEmpty()) {
        return {};
    }

    return QString("{REF:%1@I:%2}").arg(shortField, uuidStr);
}

EntryReferenceType Entry::referenceType(const QString& referenceStr)
{
    const QString referenceLowerStr = referenceStr.toLower();
    EntryReferenceType result = EntryReferenceType::Unknown;
    if (referenceLowerStr == QLatin1String("t")) {
        result = EntryReferenceType::Title;
    } else if (referenceLowerStr == QLatin1String("u")) {
        result = EntryReferenceType::UserName;
    } else if (referenceLowerStr == QLatin1String("p")) {
        result = EntryReferenceType::Password;
    } else if (referenceLowerStr == QLatin1String("a")) {
        result = EntryReferenceType::Url;
    } else if (referenceLowerStr == QLatin1String("n")) {
        result = EntryReferenceType::Notes;
    } else if (referenceLowerStr == QLatin1String("i")) {
        result = EntryReferenceType::QUuid;
    } else if (referenceLowerStr == QLatin1String("o")) {
        result = EntryReferenceType::CustomAttributes;
    }

    return result;
}

const QUuid& Entry::uuid() const
{
    return m_uuid;
}

const QString Entry::uuidToHex() const
{
    return Tools::uuidToHex(m_uuid);
}

QImage Entry::icon() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->icon(m_data.iconNumber);
    } else {
        Q_ASSERT(database());

        if (database()) {
            return database()->metadata()->customIcon(m_data.customIcon);
        } else {
            return QImage();
        }
    }
}

QPixmap Entry::iconPixmap() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    }

    Q_ASSERT(database());
    if (database()) {
        return database()->metadata()->customIconPixmap(m_data.customIcon);
    }
    return QPixmap();
}

QPixmap Entry::iconScaledPixmap() const
{
    if (m_data.customIcon.isNull()) {
        // built-in icons are 16x16 so don't need to be scaled
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    }
    Q_ASSERT(database());
    return database()->metadata()->customIconScaledPixmap(m_data.customIcon);
}

int Entry::iconNumber() const
{
    return m_data.iconNumber;
}

const QUuid& Entry::iconUuid() const
{
    return m_data.customIcon;
}

QColor Entry::foregroundColor() const
{
    return m_data.foregroundColor;
}

QColor Entry::backgroundColor() const
{
    return m_data.backgroundColor;
}

QString Entry::overrideUrl() const
{
    return m_data.overrideUrl;
}

QString Entry::tags() const
{
    return m_data.tags;
}

const TimeInfo& Entry::timeInfo() const
{
    return m_data.timeInfo;
}

bool Entry::autoTypeEnabled() const
{
    return m_data.autoTypeEnabled;
}

int Entry::autoTypeObfuscation() const
{
    return m_data.autoTypeObfuscation;
}

QString Entry::defaultAutoTypeSequence() const
{
    return m_data.defaultAutoTypeSequence;
}

/**
 * Determine the effective sequence that will be injected
 * This function return an empty string if a parent group has autotype disabled or if the entry has no parent
 */
QString Entry::effectiveAutoTypeSequence() const
{
    if (!autoTypeEnabled()) {
        return {};
    }

    const Group* parent = group();
    if (!parent) {
        return {};
    }

    QString sequence = parent->effectiveAutoTypeSequence();
    if (sequence.isEmpty()) {
        return {};
    }

    if (!m_data.defaultAutoTypeSequence.isEmpty()) {
        return m_data.defaultAutoTypeSequence;
    }

    if (sequence == Group::RootAutoTypeSequence && (!username().isEmpty() || !password().isEmpty())) {
        if (username().isEmpty()) {
            return AutoTypeSequencePassword;
        } else if (password().isEmpty()) {
            return AutoTypeSequenceUsername;
        }
        return Group::RootAutoTypeSequence;
    }

    return sequence;
}

AutoTypeAssociations* Entry::autoTypeAssociations()
{
    return m_autoTypeAssociations;
}

const AutoTypeAssociations* Entry::autoTypeAssociations() const
{
    return m_autoTypeAssociations;
}

QString Entry::title() const
{
    return m_attributes->value(EntryAttributes::TitleKey);
}

QString Entry::url() const
{
    return m_attributes->value(EntryAttributes::URLKey);
}

QString Entry::webUrl() const
{
    QString url = resolveMultiplePlaceholders(m_attributes->value(EntryAttributes::URLKey));
    return resolveUrl(url);
}

QString Entry::displayUrl() const
{
    QString url = maskPasswordPlaceholders(m_attributes->value(EntryAttributes::URLKey));
    return resolveMultiplePlaceholders(url);
}

QString Entry::username() const
{
    return m_attributes->value(EntryAttributes::UserNameKey);
}

QString Entry::password() const
{
    return m_attributes->value(EntryAttributes::PasswordKey);
}

QString Entry::notes() const
{
    return m_attributes->value(EntryAttributes::NotesKey);
}

QString Entry::attribute(const QString& key) const
{
    return m_attributes->value(key);
}

bool Entry::isExpired() const
{
    return m_data.timeInfo.expires() && m_data.timeInfo.expiryTime() < Clock::currentDateTimeUtc();
}

bool Entry::isRecycled() const
{
    const Database* db = database();
    if (!db) {
        return false;
    }

    return m_group == db->metadata()->recycleBin() || m_group->isRecycled();
}

bool Entry::isAttributeReference(const QString& key) const
{
    return m_attributes->isReference(key);
}

bool Entry::isAttributeReferenceOf(const QString& key, const QUuid& uuid) const
{
    if (!m_attributes->isReference(key)) {
        return false;
    }

    return m_attributes->value(key).contains(Tools::uuidToHex(uuid), Qt::CaseInsensitive);
}

bool Entry::hasReferences() const
{
    const QList<QString> keyList = EntryAttributes::DefaultAttributes;
    for (const QString& key : keyList) {
        if (m_attributes->isReference(key)) {
            return true;
        }
    }
    return false;
}

bool Entry::hasReferencesTo(const QUuid& uuid) const
{
    const QList<QString> keyList = EntryAttributes::DefaultAttributes;
    for (const QString& key : keyList) {
        if (isAttributeReferenceOf(key, uuid)) {
            return true;
        }
    }
    return false;
}

void Entry::replaceReferencesWithValues(const Entry* other)
{
    for (const QString& key : EntryAttributes::DefaultAttributes) {
        if (isAttributeReferenceOf(key, other->uuid())) {
            setDefaultAttribute(key, other->attribute(key));
        }
    }
}

EntryAttributes* Entry::attributes()
{
    return m_attributes;
}

const EntryAttributes* Entry::attributes() const
{
    return m_attributes;
}

EntryAttachments* Entry::attachments()
{
    return m_attachments;
}

const EntryAttachments* Entry::attachments() const
{
    return m_attachments;
}

CustomData* Entry::customData()
{
    return m_customData;
}

const CustomData* Entry::customData() const
{
    return m_customData;
}

bool Entry::hasTotp() const
{
    return !m_data.totpSettings.isNull();
}

QString Entry::totp() const
{
    if (hasTotp()) {
        return Totp::generateTotp(m_data.totpSettings);
    }
    return {};
}

void Entry::setTotp(QSharedPointer<Totp::Settings> settings)
{
    beginUpdate();
    m_attributes->remove(Totp::ATTRIBUTE_OTP);
    m_attributes->remove(Totp::ATTRIBUTE_SEED);
    m_attributes->remove(Totp::ATTRIBUTE_SETTINGS);

    if (settings->key.isEmpty()) {
        m_data.totpSettings.reset();
    } else {
        m_data.totpSettings = std::move(settings);
        auto text = Totp::writeSettings(m_data.totpSettings, title(), username());
        if (m_data.totpSettings->format != Totp::StorageFormat::LEGACY) {
            m_attributes->set(Totp::ATTRIBUTE_OTP, text, true);
        } else {
            m_attributes->set(Totp::ATTRIBUTE_SEED, m_data.totpSettings->key, true);
            m_attributes->set(Totp::ATTRIBUTE_SETTINGS, text);
        }
    }
    endUpdate();
}

void Entry::updateTotp()
{
    if (m_attributes->contains(Totp::ATTRIBUTE_SETTINGS)) {
        m_data.totpSettings = Totp::parseSettings(m_attributes->value(Totp::ATTRIBUTE_SETTINGS),
                                                  m_attributes->value(Totp::ATTRIBUTE_SEED));
    } else if (m_attributes->contains(Totp::ATTRIBUTE_OTP)) {
        m_data.totpSettings = Totp::parseSettings(m_attributes->value(Totp::ATTRIBUTE_OTP));
    }
}

QSharedPointer<Totp::Settings> Entry::totpSettings() const
{
    return m_data.totpSettings;
}

void Entry::setUuid(const QUuid& uuid)
{
    Q_ASSERT(!uuid.isNull());
    set(m_uuid, uuid);
}

void Entry::setIcon(int iconNumber)
{
    Q_ASSERT(iconNumber >= 0);

    if (m_data.iconNumber != iconNumber || !m_data.customIcon.isNull()) {
        m_data.iconNumber = iconNumber;
        m_data.customIcon = QUuid();

        emit entryModified();
        emitDataChanged();
    }
}

void Entry::setIcon(const QUuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    if (m_data.customIcon != uuid) {
        m_data.customIcon = uuid;
        m_data.iconNumber = 0;

        emit entryModified();
        emitDataChanged();
    }
}

void Entry::setForegroundColor(const QColor& color)
{
    set(m_data.foregroundColor, color);
}

void Entry::setBackgroundColor(const QColor& color)
{
    set(m_data.backgroundColor, color);
}

void Entry::setOverrideUrl(const QString& url)
{
    set(m_data.overrideUrl, url);
}

void Entry::setTags(const QString& tags)
{
    set(m_data.tags, tags);
}

void Entry::setTimeInfo(const TimeInfo& timeInfo)
{
    m_data.timeInfo = timeInfo;
}

void Entry::setAutoTypeEnabled(bool enable)
{
    set(m_data.autoTypeEnabled, enable);
}

void Entry::setAutoTypeObfuscation(int obfuscation)
{
    set(m_data.autoTypeObfuscation, obfuscation);
}

void Entry::setDefaultAutoTypeSequence(const QString& sequence)
{
    set(m_data.defaultAutoTypeSequence, sequence);
}

void Entry::setTitle(const QString& title)
{
    m_attributes->set(EntryAttributes::TitleKey, title, m_attributes->isProtected(EntryAttributes::TitleKey));
}

void Entry::setUrl(const QString& url)
{
    bool remove = url != m_attributes->value(EntryAttributes::URLKey)
                  && (m_attributes->value(EntryAttributes::RememberCmdExecAttr) == "1"
                      || m_attributes->value(EntryAttributes::RememberCmdExecAttr) == "0");
    if (remove) {
        m_attributes->remove(EntryAttributes::RememberCmdExecAttr);
    }
    m_attributes->set(EntryAttributes::URLKey, url, m_attributes->isProtected(EntryAttributes::URLKey));
}

void Entry::setUsername(const QString& username)
{
    m_attributes->set(EntryAttributes::UserNameKey, username, m_attributes->isProtected(EntryAttributes::UserNameKey));
}

void Entry::setPassword(const QString& password)
{
    m_attributes->set(EntryAttributes::PasswordKey, password, m_attributes->isProtected(EntryAttributes::PasswordKey));
}

void Entry::setNotes(const QString& notes)
{
    m_attributes->set(EntryAttributes::NotesKey, notes, m_attributes->isProtected(EntryAttributes::NotesKey));
}

void Entry::setDefaultAttribute(const QString& attribute, const QString& value)
{
    Q_ASSERT(EntryAttributes::isDefaultAttribute(attribute));

    if (!EntryAttributes::isDefaultAttribute(attribute)) {
        return;
    }

    m_attributes->set(attribute, value, m_attributes->isProtected(attribute));
}

void Entry::setExpires(const bool& value)
{
    if (m_data.timeInfo.expires() != value) {
        m_data.timeInfo.setExpires(value);
        emit entryModified();
    }
}

void Entry::setExpiryTime(const QDateTime& dateTime)
{
    if (m_data.timeInfo.expiryTime() != dateTime) {
        m_data.timeInfo.setExpiryTime(dateTime);
        emit entryModified();
    }
}

QList<Entry*> Entry::historyItems()
{
    return m_history;
}

const QList<Entry*>& Entry::historyItems() const
{
    return m_history;
}

void Entry::addHistoryItem(Entry* entry)
{
    Q_ASSERT(!entry->parent());

    m_history.append(entry);
    emit entryModified();
}

void Entry::removeHistoryItems(const QList<Entry*>& historyEntries)
{
    if (historyEntries.isEmpty()) {
        return;
    }

    for (Entry* entry : historyEntries) {
        Q_ASSERT(!entry->parent());
        Q_ASSERT(entry->uuid().isNull() || entry->uuid() == uuid());
        Q_ASSERT(m_history.contains(entry));

        m_history.removeOne(entry);
        delete entry;
    }

    emit entryModified();
}

void Entry::truncateHistory()
{
    const Database* db = database();

    if (!db) {
        return;
    }

    int histMaxItems = db->metadata()->historyMaxItems();
    if (histMaxItems > -1) {
        int historyCount = 0;
        QMutableListIterator<Entry*> i(m_history);
        i.toBack();
        while (i.hasPrevious()) {
            historyCount++;
            Entry* entry = i.previous();
            if (historyCount > histMaxItems) {
                delete entry;
                i.remove();
            }
        }
    }

    int histMaxSize = db->metadata()->historyMaxSize();
    if (histMaxSize > -1) {
        int size = 0;
        QSet<QByteArray> foundAttachments = attachments()->values();

        QMutableListIterator<Entry*> i(m_history);
        i.toBack();
        const QRegularExpression delimiter(",|:|;");
        while (i.hasPrevious()) {
            Entry* historyItem = i.previous();

            // don't calculate size if it's already above the maximum
            if (size <= histMaxSize) {
                size += historyItem->attributes()->attributesSize();
                size += historyItem->autoTypeAssociations()->associationsSize();
                size += historyItem->attachments()->attachmentsSize();
                size += historyItem->customData()->dataSize();
                const QStringList tags = historyItem->tags().split(delimiter, QString::SkipEmptyParts);
                for (const QString& tag : tags) {
                    size += tag.toUtf8().size();
                }
                foundAttachments += historyItem->attachments()->values();
            }

            if (size > histMaxSize) {
                delete historyItem;
                i.remove();
            }
        }
    }
}

bool Entry::equals(const Entry* other, CompareItemOptions options) const
{
    if (!other) {
        return false;
    }
    if (m_uuid != other->uuid()) {
        return false;
    }
    if (!m_data.equals(other->m_data, options)) {
        return false;
    }
    if (*m_customData != *other->m_customData) {
        return false;
    }
    if (*m_attributes != *other->m_attributes) {
        return false;
    }
    if (*m_attachments != *other->m_attachments) {
        return false;
    }
    if (*m_autoTypeAssociations != *other->m_autoTypeAssociations) {
        return false;
    }
    if (!options.testFlag(CompareItemIgnoreHistory)) {
        if (m_history.count() != other->m_history.count()) {
            return false;
        }
        for (int i = 0; i < m_history.count(); ++i) {
            if (!m_history[i]->equals(other->m_history[i], options)) {
                return false;
            }
        }
    }
    return true;
}

Entry* Entry::clone(CloneFlags flags) const
{
    Entry* entry = new Entry();
    entry->setUpdateTimeinfo(false);
    if (flags & CloneNewUuid) {
        entry->m_uuid = QUuid::createUuid();
    } else {
        entry->m_uuid = m_uuid;
    }
    entry->m_data = m_data;
    entry->m_customData->copyDataFrom(m_customData);
    entry->m_attributes->copyDataFrom(m_attributes);
    entry->m_attachments->copyDataFrom(m_attachments);

    if (flags & CloneUserAsRef) {
        entry->m_attributes->set(EntryAttributes::UserNameKey,
                                 buildReference(uuid(), EntryAttributes::UserNameKey),
                                 m_attributes->isProtected(EntryAttributes::UserNameKey));
    }

    if (flags & ClonePassAsRef) {
        entry->m_attributes->set(EntryAttributes::PasswordKey,
                                 buildReference(uuid(), EntryAttributes::PasswordKey),
                                 m_attributes->isProtected(EntryAttributes::PasswordKey));
    }

    entry->m_autoTypeAssociations->copyDataFrom(m_autoTypeAssociations);
    if (flags & CloneIncludeHistory) {
        for (Entry* historyItem : m_history) {
            Entry* historyItemClone =
                historyItem->clone(flags & ~CloneIncludeHistory & ~CloneNewUuid & ~CloneResetTimeInfo);
            historyItemClone->setUpdateTimeinfo(false);
            historyItemClone->setUuid(entry->uuid());
            historyItemClone->setUpdateTimeinfo(true);
            entry->addHistoryItem(historyItemClone);
        }
    }

    if (flags & CloneResetTimeInfo) {
        QDateTime now = Clock::currentDateTimeUtc();
        entry->m_data.timeInfo.setCreationTime(now);
        entry->m_data.timeInfo.setLastModificationTime(now);
        entry->m_data.timeInfo.setLastAccessTime(now);
        entry->m_data.timeInfo.setLocationChanged(now);
    }

    if (flags & CloneRenameTitle)
        entry->setTitle(tr("%1 - Clone").arg(entry->title()));

    entry->setUpdateTimeinfo(true);

    return entry;
}

void Entry::copyDataFrom(const Entry* other)
{
    setUpdateTimeinfo(false);
    m_data = other->m_data;
    m_customData->copyDataFrom(other->m_customData);
    m_attributes->copyDataFrom(other->m_attributes);
    m_attachments->copyDataFrom(other->m_attachments);
    m_autoTypeAssociations->copyDataFrom(other->m_autoTypeAssociations);
    setUpdateTimeinfo(true);
}

void Entry::beginUpdate()
{
    Q_ASSERT(m_tmpHistoryItem.isNull());

    m_tmpHistoryItem.reset(new Entry());
    m_tmpHistoryItem->setUpdateTimeinfo(false);
    m_tmpHistoryItem->m_uuid = m_uuid;
    m_tmpHistoryItem->m_data = m_data;
    m_tmpHistoryItem->m_attributes->copyDataFrom(m_attributes);
    m_tmpHistoryItem->m_attachments->copyDataFrom(m_attachments);
    m_tmpHistoryItem->m_autoTypeAssociations->copyDataFrom(m_autoTypeAssociations);

    m_modifiedSinceBegin = false;
}

bool Entry::endUpdate()
{
    Q_ASSERT(!m_tmpHistoryItem.isNull());
    if (m_modifiedSinceBegin) {
        m_tmpHistoryItem->setUpdateTimeinfo(true);
        addHistoryItem(m_tmpHistoryItem.take());
        truncateHistory();
    }

    m_tmpHistoryItem.reset();

    return m_modifiedSinceBegin;
}

void Entry::updateModifiedSinceBegin()
{
    m_modifiedSinceBegin = true;
}

QString Entry::resolveMultiplePlaceholdersRecursive(const QString& str, int maxDepth) const
{
    if (maxDepth <= 0) {
        qWarning("Maximum depth of replacement has been reached. Entry uuid: %s", uuid().toString().toLatin1().data());
        return str;
    }

    QString result = str;
    QRegExp placeholderRegEx("(\\{[^\\}]+\\})", Qt::CaseInsensitive, QRegExp::RegExp2);
    placeholderRegEx.setMinimal(true);
    int pos = 0;
    while ((pos = placeholderRegEx.indexIn(str, pos)) != -1) {
        const QString found = placeholderRegEx.cap(1);
        result.replace(found, resolvePlaceholderRecursive(found, maxDepth - 1));
        pos += placeholderRegEx.matchedLength();
    }

    if (result != str) {
        result = resolveMultiplePlaceholdersRecursive(result, maxDepth - 1);
    }

    return result;
}

QString Entry::resolvePlaceholderRecursive(const QString& placeholder, int maxDepth) const
{
    if (maxDepth <= 0) {
        qWarning("Maximum depth of replacement has been reached. Entry uuid: %s", uuid().toString().toLatin1().data());
        return placeholder;
    }

    const PlaceholderType typeOfPlaceholder = placeholderType(placeholder);
    switch (typeOfPlaceholder) {
    case PlaceholderType::NotPlaceholder:
    case PlaceholderType::Unknown:
        return resolveMultiplePlaceholdersRecursive(placeholder, maxDepth - 1);
    case PlaceholderType::Title:
        if (placeholderType(title()) == PlaceholderType::Title) {
            return title();
        }
        return resolveMultiplePlaceholdersRecursive(title(), maxDepth - 1);
    case PlaceholderType::UserName:
        if (placeholderType(username()) == PlaceholderType::UserName) {
            return username();
        }
        return resolveMultiplePlaceholdersRecursive(username(), maxDepth - 1);
    case PlaceholderType::Password:
        if (placeholderType(password()) == PlaceholderType::Password) {
            return password();
        }
        return resolveMultiplePlaceholdersRecursive(password(), maxDepth - 1);
    case PlaceholderType::Notes:
        if (placeholderType(notes()) == PlaceholderType::Notes) {
            return notes();
        }
        return resolveMultiplePlaceholdersRecursive(notes(), maxDepth - 1);
    case PlaceholderType::Url:
        if (placeholderType(url()) == PlaceholderType::Url) {
            return url();
        }
        return resolveMultiplePlaceholdersRecursive(url(), maxDepth - 1);
    case PlaceholderType::UrlWithoutScheme:
    case PlaceholderType::UrlScheme:
    case PlaceholderType::UrlHost:
    case PlaceholderType::UrlPort:
    case PlaceholderType::UrlPath:
    case PlaceholderType::UrlQuery:
    case PlaceholderType::UrlFragment:
    case PlaceholderType::UrlUserInfo:
    case PlaceholderType::UrlUserName:
    case PlaceholderType::UrlPassword: {
        const QString strUrl = resolveMultiplePlaceholdersRecursive(url(), maxDepth - 1);
        return resolveUrlPlaceholder(strUrl, typeOfPlaceholder);
    }
    case PlaceholderType::Totp:
        // totp can't have placeholder inside
        return totp();
    case PlaceholderType::CustomAttribute: {
        const QString key = placeholder.mid(3, placeholder.length() - 4); // {S:attr} => mid(3, len - 4)
        return attributes()->hasKey(key) ? attributes()->value(key) : QString();
    }
    case PlaceholderType::Reference:
        return resolveReferencePlaceholderRecursive(placeholder, maxDepth);
    }

    return placeholder;
}

QString Entry::resolveReferencePlaceholderRecursive(const QString& placeholder, int maxDepth) const
{
    if (maxDepth <= 0) {
        qWarning("Maximum depth of replacement has been reached. Entry uuid: %s", uuid().toString().toLatin1().data());
        return placeholder;
    }

    // resolving references in format: {REF:<WantedField>@<SearchIn>:<SearchText>}
    // using format from http://keepass.info/help/base/fieldrefs.html at the time of writing

    QRegularExpressionMatch match = EntryAttributes::matchReference(placeholder);
    if (!match.hasMatch()) {
        return placeholder;
    }

    QString result;
    const QString searchIn = match.captured(EntryAttributes::SearchInGroupName);
    const QString searchText = match.captured(EntryAttributes::SearchTextGroupName);

    const EntryReferenceType searchInType = Entry::referenceType(searchIn);

    Q_ASSERT(m_group);
    Q_ASSERT(m_group->database());
    const Entry* refEntry = m_group->database()->rootGroup()->findEntryBySearchTerm(searchText, searchInType);

    if (refEntry) {
        const QString wantedField = match.captured(EntryAttributes::WantedFieldGroupName);
        result = refEntry->referenceFieldValue(Entry::referenceType(wantedField));

        // Referencing fields of other entries only works with standard fields, not with custom user strings.
        // If you want to reference a custom user string, you need to place a redirection in a standard field
        // of the entry with the custom string, using {S:<Name>}, and reference the standard field.
        result = refEntry->resolveMultiplePlaceholdersRecursive(result, maxDepth - 1);
    }

    return result;
}

QString Entry::referenceFieldValue(EntryReferenceType referenceType) const
{
    switch (referenceType) {
    case EntryReferenceType::Title:
        return title();
    case EntryReferenceType::UserName:
        return username();
    case EntryReferenceType::Password:
        return password();
    case EntryReferenceType::Url:
        return url();
    case EntryReferenceType::Notes:
        return notes();
    case EntryReferenceType::QUuid:
        return uuidToHex();
    default:
        break;
    }
    return QString();
}

Group* Entry::group()
{
    return m_group;
}

const Group* Entry::group() const
{
    return m_group;
}

void Entry::setGroup(Group* group)
{
    Q_ASSERT(group);

    if (m_group == group) {
        return;
    }

    if (m_group) {
        m_group->removeEntry(this);
        if (m_group->database() && m_group->database() != group->database()) {
            m_group->database()->addDeletedObject(m_uuid);

            // copy custom icon to the new database
            if (!iconUuid().isNull() && group->database()
                && m_group->database()->metadata()->containsCustomIcon(iconUuid())
                && !group->database()->metadata()->containsCustomIcon(iconUuid())) {
                group->database()->metadata()->addCustomIcon(iconUuid(), icon());
            }
        }
    }

    m_group = group;
    group->addEntry(this);

    QObject::setParent(group);

    if (m_updateTimeinfo) {
        m_data.timeInfo.setLocationChanged(Clock::currentDateTimeUtc());
    }
}

void Entry::emitDataChanged()
{
    emit entryDataChanged(this);
}

const Database* Entry::database() const
{
    if (m_group) {
        return m_group->database();
    }
    return nullptr;
}

Database* Entry::database()
{
    if (m_group) {
        return m_group->database();
    }
    return nullptr;
}

QString Entry::maskPasswordPlaceholders(const QString& str) const
{
    QString result = str;
    result.replace(QRegExp("(\\{PASSWORD\\})", Qt::CaseInsensitive, QRegExp::RegExp2), "******");
    return result;
}

Entry* Entry::resolveReference(const QString& str) const
{
    QRegularExpressionMatch match = EntryAttributes::matchReference(str);
    if (!match.hasMatch()) {
        return nullptr;
    }

    const QString searchIn = match.captured(EntryAttributes::SearchInGroupName);
    const QString searchText = match.captured(EntryAttributes::SearchTextGroupName);

    const EntryReferenceType searchInType = Entry::referenceType(searchIn);
    return m_group->database()->rootGroup()->findEntryBySearchTerm(searchText, searchInType);
}

QString Entry::resolveMultiplePlaceholders(const QString& str) const
{
    return resolveMultiplePlaceholdersRecursive(str, ResolveMaximumDepth);
}

QString Entry::resolvePlaceholder(const QString& placeholder) const
{
    return resolvePlaceholderRecursive(placeholder, ResolveMaximumDepth);
}

QString Entry::resolveUrlPlaceholder(const QString& str, Entry::PlaceholderType placeholderType) const
{
    if (str.isEmpty())
        return QString();

    const QUrl qurl(str);
    switch (placeholderType) {
    case PlaceholderType::UrlWithoutScheme:
        return qurl.toString(QUrl::RemoveScheme | QUrl::FullyDecoded);
    case PlaceholderType::UrlScheme:
        return qurl.scheme();
    case PlaceholderType::UrlHost:
        return qurl.host();
    case PlaceholderType::UrlPort:
        return QString::number(qurl.port());
    case PlaceholderType::UrlPath:
        return qurl.path();
    case PlaceholderType::UrlQuery:
        return qurl.query();
    case PlaceholderType::UrlFragment:
        return qurl.fragment();
    case PlaceholderType::UrlUserInfo:
        return qurl.userInfo();
    case PlaceholderType::UrlUserName:
        return qurl.userName();
    case PlaceholderType::UrlPassword:
        return qurl.password();
    default: {
        Q_ASSERT_X(false, "Entry::resolveUrlPlaceholder", "Bad url placeholder type");
        break;
    }
    }

    return QString();
}

Entry::PlaceholderType Entry::placeholderType(const QString& placeholder) const
{
    if (!placeholder.startsWith(QLatin1Char('{')) || !placeholder.endsWith(QLatin1Char('}'))) {
        return PlaceholderType::NotPlaceholder;
    }
    if (placeholder.startsWith(QLatin1Literal("{S:"))) {
        return PlaceholderType::CustomAttribute;
    }
    if (placeholder.startsWith(QLatin1Literal("{REF:"))) {
        return PlaceholderType::Reference;
    }

    static const QMap<QString, PlaceholderType> placeholders{
        {QStringLiteral("{TITLE}"), PlaceholderType::Title},
        {QStringLiteral("{USERNAME}"), PlaceholderType::UserName},
        {QStringLiteral("{PASSWORD}"), PlaceholderType::Password},
        {QStringLiteral("{NOTES}"), PlaceholderType::Notes},
        {QStringLiteral("{TOTP}"), PlaceholderType::Totp},
        {QStringLiteral("{URL}"), PlaceholderType::Url},
        {QStringLiteral("{URL:RMVSCM}"), PlaceholderType::UrlWithoutScheme},
        {QStringLiteral("{URL:WITHOUTSCHEME}"), PlaceholderType::UrlWithoutScheme},
        {QStringLiteral("{URL:SCM}"), PlaceholderType::UrlScheme},
        {QStringLiteral("{URL:SCHEME}"), PlaceholderType::UrlScheme},
        {QStringLiteral("{URL:HOST}"), PlaceholderType::UrlHost},
        {QStringLiteral("{URL:PORT}"), PlaceholderType::UrlPort},
        {QStringLiteral("{URL:PATH}"), PlaceholderType::UrlPath},
        {QStringLiteral("{URL:QUERY}"), PlaceholderType::UrlQuery},
        {QStringLiteral("{URL:FRAGMENT}"), PlaceholderType::UrlFragment},
        {QStringLiteral("{URL:USERINFO}"), PlaceholderType::UrlUserInfo},
        {QStringLiteral("{URL:USERNAME}"), PlaceholderType::UrlUserName},
        {QStringLiteral("{URL:PASSWORD}"), PlaceholderType::UrlPassword}};

    return placeholders.value(placeholder.toUpper(), PlaceholderType::Unknown);
}

QString Entry::resolveUrl(const QString& url) const
{
    QString newUrl = url;

    QRegExp fileRegEx("^([a-z]:)?[\\\\/]", Qt::CaseInsensitive, QRegExp::RegExp2);
    if (fileRegEx.indexIn(newUrl) != -1) {
        // Match possible file paths without the scheme and convert it to a file URL
        newUrl = QDir::fromNativeSeparators(newUrl);
        newUrl = QUrl::fromLocalFile(newUrl).toString();
    } else if (newUrl.startsWith("cmd://")) {
        QStringList cmdList = newUrl.split(" ");
        for (int i = 1; i < cmdList.size(); ++i) {
            // Don't pass arguments to the resolveUrl function (they look like URL's)
            if (!cmdList[i].startsWith("-") && !cmdList[i].startsWith("/")) {
                return resolveUrl(cmdList[i].remove(QRegExp("'|\"")));
            }
        }

        // No URL in this command
        return QString("");
    }

    if (!newUrl.isEmpty() && !newUrl.contains("://")) {
        // URL doesn't have a protocol, add https by default
        newUrl.prepend("https://");
    }

    // Validate the URL
    QUrl tempUrl = QUrl(newUrl);
    if (tempUrl.isValid()
        && (tempUrl.scheme() == "http" || tempUrl.scheme() == "https" || tempUrl.scheme() == "file")) {
        return tempUrl.url();
    }

    // No valid http URL's found
    return QString("");
}

bool EntryData::operator==(const EntryData& other) const
{
    return equals(other, CompareItemDefault);
}

bool EntryData::operator!=(const EntryData& other) const
{
    return !(*this == other);
}

bool EntryData::equals(const EntryData& other, CompareItemOptions options) const
{
    if (::compare(iconNumber, other.iconNumber, options) != 0) {
        return false;
    }
    if (::compare(customIcon, other.customIcon, options) != 0) {
        return false;
    }
    if (::compare(foregroundColor, other.foregroundColor, options) != 0) {
        return false;
    }
    if (::compare(backgroundColor, other.backgroundColor, options) != 0) {
        return false;
    }
    if (::compare(overrideUrl, other.overrideUrl, options) != 0) {
        return false;
    }
    if (::compare(tags, other.tags, options) != 0) {
        return false;
    }
    if (::compare(autoTypeEnabled, other.autoTypeEnabled, options) != 0) {
        return false;
    }
    if (::compare(autoTypeObfuscation, other.autoTypeObfuscation, options) != 0) {
        return false;
    }
    if (::compare(defaultAutoTypeSequence, other.defaultAutoTypeSequence, options) != 0) {
        return false;
    }
    if (!timeInfo.equals(other.timeInfo, options)) {
        return false;
    }
    if (!totpSettings.isNull() && !other.totpSettings.isNull()) {
        // Both have TOTP settings, compare them
        if (::compare(totpSettings->key, other.totpSettings->key, options) != 0) {
            return false;
        }
        if (::compare(totpSettings->digits, other.totpSettings->digits, options) != 0) {
            return false;
        }
        if (::compare(totpSettings->step, other.totpSettings->step, options) != 0) {
            return false;
        }
    } else if (totpSettings.isNull() != other.totpSettings.isNull()) {
        // The existance of TOTP has changed between these entries
        return false;
    }

    return true;
}
