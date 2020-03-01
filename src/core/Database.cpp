/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Database.h"

#include "core/Clock.h"
#include "core/FileWatcher.h"
#include "core/Group.h"
#include "core/Merger.h"
#include "core/Metadata.h"
#include "format/KdbxXmlReader.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTimer>
#include <QXmlStreamReader>

QHash<QUuid, QPointer<Database>> Database::s_uuidMap;

Database::Database()
    : m_metadata(new Metadata(this))
    , m_data()
    , m_rootGroup(nullptr)
    , m_fileWatcher(new FileWatcher(this))
    , m_emitModified(false)
    , m_uuid(QUuid::createUuid())
{
    setRootGroup(new Group());
    rootGroup()->setUuid(QUuid::createUuid());
    rootGroup()->setName(tr("Root", "Root group name"));
    m_modifiedTimer.setSingleShot(true);

    s_uuidMap.insert(m_uuid, this);

    connect(m_metadata, SIGNAL(metadataModified()), SLOT(markAsModified()));
    connect(&m_modifiedTimer, SIGNAL(timeout()), SIGNAL(databaseModified()));
    connect(this, SIGNAL(databaseOpened()), SLOT(updateCommonUsernames()));
    connect(this, SIGNAL(databaseSaved()), SLOT(updateCommonUsernames()));
    connect(m_fileWatcher, SIGNAL(fileChanged()), SIGNAL(databaseFileChanged()));

    m_modified = false;
    m_emitModified = true;
}

Database::Database(const QString& filePath)
    : Database()
{
    setFilePath(filePath);
}

Database::~Database()
{
    releaseData();
}

QUuid Database::uuid() const
{
    return m_uuid;
}

/**
 * Open the database from a previously specified file.
 * Unless `readOnly` is set to false, the database will be opened in
 * read-write mode and fall back to read-only if that is not possible.
 *
 * @param key composite key for unlocking the database
 * @param readOnly open in read-only mode
 * @param error error message in case of failure
 * @return true on success
 */
bool Database::open(QSharedPointer<const CompositeKey> key, QString* error, bool readOnly)
{
    Q_ASSERT(!m_data.filePath.isEmpty());
    if (m_data.filePath.isEmpty()) {
        return false;
    }
    return open(m_data.filePath, std::move(key), error, readOnly);
}

/**
 * Open the database from a file.
 * Unless `readOnly` is set to false, the database will be opened in
 * read-write mode and fall back to read-only if that is not possible.
 *
 * @param filePath path to the file
 * @param key composite key for unlocking the database
 * @param readOnly open in read-only mode
 * @param error error message in case of failure
 * @return true on success
 */
bool Database::open(const QString& filePath, QSharedPointer<const CompositeKey> key, QString* error, bool readOnly)
{
    if (isInitialized() && m_modified) {
        emit databaseDiscarded();
    }

    m_initialized = false;
    setEmitModified(false);

    QFile dbFile(filePath);
    if (!dbFile.exists()) {
        if (error) {
            *error = tr("File %1 does not exist.").arg(filePath);
        }
        return false;
    }

    if (!readOnly && !dbFile.open(QIODevice::ReadWrite)) {
        readOnly = true;
    }

    if (!dbFile.isOpen() && !dbFile.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = tr("Unable to open file %1.").arg(filePath);
        }
        return false;
    }

    KeePass2Reader reader;
    if (!reader.readDatabase(&dbFile, std::move(key), this)) {
        if (error) {
            *error = tr("Error while reading the database: %1").arg(reader.errorString());
        }
        return false;
    }

    setReadOnly(readOnly);
    setFilePath(filePath);
    dbFile.close();

    markAsClean();

    m_initialized = true;
    emit databaseOpened();
    m_fileWatcher->start(canonicalFilePath(), 30, 1);
    setEmitModified(true);

    return true;
}

/**
 * Save the database to the current file path. It is an error to call this function
 * if no file path has been defined.
 *
 * @param error error message in case of failure
 * @param atomic Use atomic file transactions
 * @param backup Backup the existing database file, if exists
 * @return true on success
 */
bool Database::save(QString* error, bool atomic, bool backup)
{
    Q_ASSERT(!m_data.filePath.isEmpty());
    if (m_data.filePath.isEmpty()) {
        if (error) {
            *error = tr("Could not save, database does not point to a valid file.");
        }
        return false;
    }

    return saveAs(m_data.filePath, error, atomic, backup);
}

/**
 * Save the database to a specific file.
 *
 * If atomic is false, this function uses QTemporaryFile instead of QSaveFile
 * due to a bug in Qt (https://bugreports.qt.io/browse/QTBUG-57299) that may
 * prevent the QSaveFile from renaming itself when using Dropbox, Google Drive,
 * or OneDrive.
 *
 * The risk in using QTemporaryFile is that the rename function is not atomic
 * and may result in loss of data if there is a crash or power loss at the
 * wrong moment.
 *
 * @param filePath Absolute path of the file to save
 * @param error error message in case of failure
 * @param atomic Use atomic file transactions
 * @param backup Backup the existing database file, if exists
 * @return true on success
 */
bool Database::saveAs(const QString& filePath, QString* error, bool atomic, bool backup)
{
    if (filePath == m_data.filePath) {
        // Disallow saving to the same file if read-only
        if (m_data.isReadOnly) {
            Q_ASSERT_X(false, "Database::saveAs", "Could not save, database file is read-only.");
            if (error) {
                *error = tr("Could not save, database file is read-only.");
            }
            return false;
        }

        // Fail-safe check to make sure we don't overwrite underlying file changes
        // that have not yet triggered a file reload/merge operation.
        if (!m_fileWatcher->hasSameFileChecksum()) {
            if (error) {
                *error = tr("Database file has unmerged changes.");
            }
            return false;
        }
    }

    // Clear read-only flag
    setReadOnly(false);
    m_fileWatcher->stop();

    auto& canonicalFilePath = QFileInfo::exists(filePath) ? QFileInfo(filePath).canonicalFilePath() : filePath;
    bool ok = performSave(canonicalFilePath, error, atomic, backup);
    if (ok) {
        markAsClean();
        setFilePath(filePath);
        m_fileWatcher->start(canonicalFilePath, 30, 1);
    } else {
        // Saving failed, don't rewatch file since it does not represent our database
        markAsModified();
    }

    return ok;
}

bool Database::performSave(const QString& filePath, QString* error, bool atomic, bool backup)
{
    if (atomic) {
        QSaveFile saveFile(filePath);
        if (saveFile.open(QIODevice::WriteOnly)) {
            // write the database to the file
            if (!writeDatabase(&saveFile, error)) {
                return false;
            }

            if (backup) {
                backupDatabase(filePath);
            }

            if (saveFile.commit()) {
                // successfully saved database file
                return true;
            }
        }

        if (error) {
            *error = saveFile.errorString();
        }
    } else {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            // write the database to the file
            if (!writeDatabase(&tempFile, error)) {
                return false;
            }

            tempFile.close(); // flush to disk

            if (backup) {
                backupDatabase(filePath);
            }

            // Delete the original db and move the temp file in place
            QFile::remove(filePath);

            // Note: call into the QFile rename instead of QTemporaryFile
            // due to an undocumented difference in how the function handles
            // errors. This prevents errors when saving across file systems.
            if (tempFile.QFile::rename(filePath)) {
                // successfully saved the database
                tempFile.setAutoRemove(false);
                return true;
            } else if (!backup || !restoreDatabase(filePath)) {
                // Failed to copy new database in place, and
                // failed to restore from backup or backups disabled
                tempFile.setAutoRemove(false);
                if (error) {
                    *error = tr("%1\nBackup database located at %2").arg(tempFile.errorString(), tempFile.fileName());
                }
                return false;
            }
        }

        if (error) {
            *error = tempFile.errorString();
        }
    }

    // Saving failed
    return false;
}

bool Database::writeDatabase(QIODevice* device, QString* error)
{
    Q_ASSERT(!m_data.isReadOnly);
    if (m_data.isReadOnly) {
        if (error) {
            *error = tr("File cannot be written as it is opened in read-only mode.");
        }
        return false;
    }

    PasswordKey oldTransformedKey;
    if (m_data.hasKey) {
        oldTransformedKey.setHash(m_data.transformedMasterKey->rawKey());
    }

    KeePass2Writer writer;
    setEmitModified(false);
    writer.writeDatabase(device, this);
    setEmitModified(true);

    if (writer.hasError()) {
        if (error) {
            *error = writer.errorString();
        }
        return false;
    }

    QByteArray newKey = m_data.transformedMasterKey->rawKey();
    Q_ASSERT(!newKey.isEmpty());
    Q_ASSERT(newKey != oldTransformedKey.rawKey());
    if (newKey.isEmpty() || newKey == oldTransformedKey.rawKey()) {
        if (error) {
            *error = tr("Key not transformed. This is a bug, please report it to the developers!");
        }
        return false;
    }

    return true;
}

bool Database::extract(QByteArray& xmlOutput, QString* error)
{
    KeePass2Writer writer;
    writer.extractDatabase(this, xmlOutput);
    if (writer.hasError()) {
        if (error) {
            *error = writer.errorString();
        }
        return false;
    }

    return true;
}

bool Database::import(const QString& xmlExportPath, QString* error)
{
    KdbxXmlReader reader(KeePass2::FILE_VERSION_4);
    QFile file(xmlExportPath);
    file.open(QIODevice::ReadOnly);

    reader.readDatabase(&file, this);

    if (reader.hasError()) {
        if (error) {
            *error = reader.errorString();
        }
        return false;
    }

    return true;
}

/**
 * Release all stored group, entry, and meta data of this database.
 *
 * Call this method to ensure all data is cleared even if valid
 * pointers to this Database object are still being held.
 *
 * A previously reparented root group will not be freed.
 */

void Database::releaseData()
{
    s_uuidMap.remove(m_uuid);
    m_uuid = QUuid();

    if (m_modified) {
        emit databaseDiscarded();
    }

    m_data.clear();

    if (m_rootGroup && m_rootGroup->parent() == this) {
        delete m_rootGroup;
    }
    if (m_metadata) {
        delete m_metadata;
    }
    if (m_fileWatcher) {
        delete m_fileWatcher;
    }

    m_deletedObjects.clear();
    m_commonUsernames.clear();

    m_initialized = false;
    m_modified = false;
}

/**
 * Remove the old backup and replace it with a new one
 * backups are named <filename>.old.<extension>
 *
 * @param filePath Path to the file to backup
 * @return true on success
 */
bool Database::backupDatabase(const QString& filePath)
{
    static auto re = QRegularExpression("(\\.[^.]+)$");

    auto match = re.match(filePath);
    auto backupFilePath = filePath;
    backupFilePath = backupFilePath.replace(re, "") + ".old" + match.captured(1);
    QFile::remove(backupFilePath);
    return QFile::copy(filePath, backupFilePath);
}

/**
 * Restores the database file from the backup file with
 * name <filename>.old.<extension> to filePath. This will
 * overwrite the existing file!
 *
 * @param filePath Path to the file to restore
 * @return true on success
 */
bool Database::restoreDatabase(const QString& filePath)
{
    static auto re = QRegularExpression("^(.*?)(\\.[^.]+)?$");

    auto match = re.match(filePath);
    auto backupFilePath = match.captured(1) + ".old" + match.captured(2);
    // Only try to restore if the backup file actually exists
    if (QFile::exists(backupFilePath)) {
        QFile::remove(filePath);
        return QFile::copy(backupFilePath, filePath);
    }
    return false;
}

bool Database::isReadOnly() const
{
    return m_data.isReadOnly;
}

void Database::setReadOnly(bool readOnly)
{
    m_data.isReadOnly = readOnly;
}

/**
 * Returns true if database has been fully decrypted and populated, i.e. if
 * it's not just an empty default instance.
 *
 * @return true if database has been fully initialized
 */
bool Database::isInitialized() const
{
    return m_initialized;
}

/**
 * @param initialized true to mark database as initialized
 */
void Database::setInitialized(bool initialized)
{
    m_initialized = initialized;
}

Group* Database::rootGroup()
{
    return m_rootGroup;
}

const Group* Database::rootGroup() const
{
    return m_rootGroup;
}

/**
 * Sets group as the root group and takes ownership of it.
 * Warning: Be careful when calling this method as it doesn't
 *          emit any notifications so e.g. models aren't updated.
 *          The caller is responsible for cleaning up the previous
            root group.
 */
void Database::setRootGroup(Group* group)
{
    Q_ASSERT(group);

    if (isInitialized() && m_modified) {
        emit databaseDiscarded();
    }

    m_rootGroup = group;
    m_rootGroup->setParent(this);
}

Metadata* Database::metadata()
{
    return m_metadata;
}

const Metadata* Database::metadata() const
{
    return m_metadata;
}

/**
 * Returns the original file path that was provided for
 * this database. This path may not exist, may contain
 * unresolved symlinks, or have malformed slashes.
 *
 * @return original file path
 */
QString Database::filePath() const
{
    return m_data.filePath;
}

/**
 * Returns the canonical file path of this databases'
 * set file path. This returns an empty string if the
 * file does not exist or cannot be resolved.
 *
 * @return canonical file path
 */
QString Database::canonicalFilePath() const
{
    QFileInfo fileInfo(m_data.filePath);
    return fileInfo.canonicalFilePath();
}

void Database::setFilePath(const QString& filePath)
{
    if (filePath != m_data.filePath) {
        QString oldPath = m_data.filePath;
        m_data.filePath = filePath;
        // Don't watch for changes until the next open or save operation
        m_fileWatcher->stop();
        emit filePathChanged(oldPath, filePath);
    }
}

QList<DeletedObject> Database::deletedObjects()
{
    return m_deletedObjects;
}

const QList<DeletedObject>& Database::deletedObjects() const
{
    return m_deletedObjects;
}

bool Database::containsDeletedObject(const QUuid& uuid) const
{
    for (const DeletedObject& currentObject : m_deletedObjects) {
        if (currentObject.uuid == uuid) {
            return true;
        }
    }
    return false;
}

bool Database::containsDeletedObject(const DeletedObject& object) const
{
    for (const DeletedObject& currentObject : m_deletedObjects) {
        if (currentObject.uuid == object.uuid) {
            return true;
        }
    }
    return false;
}

void Database::setDeletedObjects(const QList<DeletedObject>& delObjs)
{
    if (m_deletedObjects == delObjs) {
        return;
    }
    m_deletedObjects = delObjs;
}

void Database::addDeletedObject(const DeletedObject& delObj)
{
    Q_ASSERT(delObj.deletionTime.timeSpec() == Qt::UTC);
    m_deletedObjects.append(delObj);
}

void Database::addDeletedObject(const QUuid& uuid)
{
    DeletedObject delObj;
    delObj.deletionTime = Clock::currentDateTimeUtc();
    delObj.uuid = uuid;

    addDeletedObject(delObj);
}

QList<QString> Database::commonUsernames()
{
    return m_commonUsernames;
}

void Database::updateCommonUsernames(int topN)
{
    m_commonUsernames.clear();
    m_commonUsernames.append(rootGroup()->usernamesRecursive(topN));
}

const QUuid& Database::cipher() const
{
    return m_data.cipher;
}

Database::CompressionAlgorithm Database::compressionAlgorithm() const
{
    return m_data.compressionAlgorithm;
}

QByteArray Database::transformedMasterKey() const
{
    return m_data.transformedMasterKey->rawKey();
}

QByteArray Database::challengeResponseKey() const
{
    return m_data.challengeResponseKey->rawKey();
}

bool Database::challengeMasterSeed(const QByteArray& masterSeed)
{
    if (m_data.key) {
        m_data.masterSeed->setHash(masterSeed);
        QByteArray response;
        bool ok = m_data.key->challenge(masterSeed, response);
        if (ok && !response.isEmpty()) {
            m_data.challengeResponseKey->setHash(response);
        } else if (ok && response.isEmpty()) {
            // no CR key present, make sure buffer is empty
            m_data.challengeResponseKey.reset(new PasswordKey);
        }
        return ok;
    }
    return false;
}

void Database::setCipher(const QUuid& cipher)
{
    Q_ASSERT(!cipher.isNull());

    m_data.cipher = cipher;
}

void Database::setCompressionAlgorithm(Database::CompressionAlgorithm algo)
{
    Q_ASSERT(static_cast<quint32>(algo) <= CompressionAlgorithmMax);

    m_data.compressionAlgorithm = algo;
}

/**
 * Set and transform a new encryption key.
 *
 * @param key key to set and transform or nullptr to reset the key
 * @param updateChangedTime true to update database change time
 * @param updateTransformSalt true to update the transform salt
 * @param transformKey trigger the KDF after setting the key
 * @return true on success
 */
bool Database::setKey(const QSharedPointer<const CompositeKey>& key,
                      bool updateChangedTime,
                      bool updateTransformSalt,
                      bool transformKey)
{
    Q_ASSERT(!m_data.isReadOnly);

    if (!key) {
        m_data.key.reset();
        m_data.transformedMasterKey.reset(new PasswordKey());
        m_data.hasKey = false;
        return true;
    }

    if (updateTransformSalt) {
        m_data.kdf->randomizeSeed();
        Q_ASSERT(!m_data.kdf->seed().isEmpty());
    }

    PasswordKey oldTransformedMasterKey;
    if (m_data.hasKey) {
        oldTransformedMasterKey.setHash(m_data.transformedMasterKey->rawKey());
    }

    QByteArray transformedMasterKey;

    if (!transformKey) {
        transformedMasterKey = QByteArray(oldTransformedMasterKey.rawKey());
    } else if (!key->transform(*m_data.kdf, transformedMasterKey)) {
        return false;
    }

    m_data.key = key;
    if (!transformedMasterKey.isEmpty()) {
        m_data.transformedMasterKey->setHash(transformedMasterKey);
    }
    m_data.hasKey = true;
    if (updateChangedTime) {
        m_metadata->setMasterKeyChanged(Clock::currentDateTimeUtc());
    }

    if (oldTransformedMasterKey.rawKey() != m_data.transformedMasterKey->rawKey()) {
        markAsModified();
    }

    return true;
}

bool Database::hasKey() const
{
    return m_data.hasKey;
}

bool Database::verifyKey(const QSharedPointer<CompositeKey>& key) const
{
    Q_ASSERT(hasKey());

    if (!m_data.challengeResponseKey->rawKey().isEmpty()) {
        QByteArray result;

        if (!key->challenge(m_data.masterSeed->rawKey(), result)) {
            // challenge failed, (YubiKey?) removed?
            return false;
        }

        if (m_data.challengeResponseKey->rawKey() != result) {
            // wrong response from challenged device(s)
            return false;
        }
    }

    return (m_data.key->rawKey() == key->rawKey());
}

QVariantMap& Database::publicCustomData()
{
    return m_data.publicCustomData;
}

const QVariantMap& Database::publicCustomData() const
{
    return m_data.publicCustomData;
}

void Database::setPublicCustomData(const QVariantMap& customData)
{
    Q_ASSERT(!m_data.isReadOnly);
    m_data.publicCustomData = customData;
}

void Database::createRecycleBin()
{
    Q_ASSERT(!m_data.isReadOnly);

    auto recycleBin = new Group();
    recycleBin->setUuid(QUuid::createUuid());
    recycleBin->setParent(rootGroup());
    recycleBin->setName(tr("Recycle Bin"));
    recycleBin->setIcon(Group::RecycleBinIconNumber);
    recycleBin->setSearchingEnabled(Group::Disable);
    recycleBin->setAutoTypeEnabled(Group::Disable);

    m_metadata->setRecycleBin(recycleBin);
}

void Database::recycleEntry(Entry* entry)
{
    Q_ASSERT(!m_data.isReadOnly);
    if (m_metadata->recycleBinEnabled()) {
        if (!m_metadata->recycleBin()) {
            createRecycleBin();
        }
        entry->setGroup(metadata()->recycleBin());
    } else {
        delete entry;
    }
}

void Database::recycleGroup(Group* group)
{
    Q_ASSERT(!m_data.isReadOnly);
    if (m_metadata->recycleBinEnabled()) {
        if (!m_metadata->recycleBin()) {
            createRecycleBin();
        }
        group->setParent(metadata()->recycleBin());
    } else {
        delete group;
    }
}

void Database::emptyRecycleBin()
{
    Q_ASSERT(!m_data.isReadOnly);
    if (m_metadata->recycleBinEnabled() && m_metadata->recycleBin()) {
        // destroying direct entries of the recycle bin
        QList<Entry*> subEntries = m_metadata->recycleBin()->entries();
        for (Entry* entry : subEntries) {
            delete entry;
        }
        // destroying direct subgroups of the recycle bin
        QList<Group*> subGroups = m_metadata->recycleBin()->children();
        for (Group* group : subGroups) {
            delete group;
        }
    }
}

void Database::setEmitModified(bool value)
{
    if (m_emitModified && !value) {
        m_modifiedTimer.stop();
    }

    m_emitModified = value;
}

bool Database::isModified() const
{
    return m_modified;
}

void Database::markAsModified()
{
    m_modified = true;
    if (m_emitModified && !m_modifiedTimer.isActive()) {
        // Small time delay prevents numerous consecutive saves due to repeated signals
        m_modifiedTimer.start(150);
    }
}

void Database::markAsClean()
{
    bool emitSignal = m_modified;
    m_modified = false;
    m_modifiedTimer.stop();
    if (emitSignal) {
        emit databaseSaved();
    }
}

/**
 * @param uuid UUID of the database
 * @return pointer to the database or nullptr if no such database exists
 */
Database* Database::databaseByUuid(const QUuid& uuid)
{
    return s_uuidMap.value(uuid, nullptr);
}

QSharedPointer<const CompositeKey> Database::key() const
{
    return m_data.key;
}

QSharedPointer<Kdf> Database::kdf() const
{
    return m_data.kdf;
}

void Database::setKdf(QSharedPointer<Kdf> kdf)
{
    Q_ASSERT(!m_data.isReadOnly);
    m_data.kdf = std::move(kdf);
}

bool Database::changeKdf(const QSharedPointer<Kdf>& kdf)
{
    Q_ASSERT(!m_data.isReadOnly);

    kdf->randomizeSeed();
    QByteArray transformedMasterKey;
    if (!m_data.key) {
        m_data.key = QSharedPointer<CompositeKey>::create();
    }
    if (!m_data.key->transform(*kdf, transformedMasterKey)) {
        return false;
    }

    setKdf(kdf);
    m_data.transformedMasterKey->setHash(transformedMasterKey);
    markAsModified();

    return true;
}
