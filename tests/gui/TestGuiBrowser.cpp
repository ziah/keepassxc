/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestGuiBrowser.h"
#include "TestGlobal.h"
#include "gui/Application.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QToolBar>

#include "config-keepassx-tests.h"
#include "core/Bootstrap.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "gui/DatabaseTabWidget.h"
#include "gui/DatabaseWidget.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/PasswordEdit.h"
#include "gui/dbsettings/DatabaseSettingsDialog.h"
#include "gui/entry/EditEntryWidget.h"
#include "gui/entry/EntryView.h"

QTEST_MAIN(TestGuiBrowser)

void TestGuiBrowser::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    // Disable autosave so we can test the modified file indicator
    config()->set("AutoSaveAfterEveryChange", false);
    config()->set("AutoSaveOnExit", false);
    // Enable the tray icon so we can test hiding/restoring the windowQByteArray
    config()->set("GUI/ShowTrayIcon", true);
    // Disable advanced settings mode (activate within individual tests to test advanced settings)
    config()->set("GUI/AdvancedSettings", false);
    // Disable the update check first time alert
    config()->set("UpdateCheckMessageShown", true);

    m_mainWindow.reset(new MainWindow());
    Bootstrap::restoreMainWindowState(*m_mainWindow);
    m_tabWidget = m_mainWindow->findChild<DatabaseTabWidget*>("tabWidget");
    m_mainWindow->show();

    // Load the NewDatabase.kdbx file into temporary storage
    QFile sourceDbFile(QString(KEEPASSX_TEST_DATA_DIR).append("/NewDatabaseBrowser.kdbx"));
    QVERIFY(sourceDbFile.open(QIODevice::ReadOnly));
    QVERIFY(Tools::readAllFromDevice(&sourceDbFile, m_dbData));
    sourceDbFile.close();
}

// Every test starts with opening the temp database
void TestGuiBrowser::init()
{
    m_dbFile.reset(new TemporaryFile());
    // Write the temp storage to a temp database file for use in our tests
    QVERIFY(m_dbFile->open());
    QCOMPARE(m_dbFile->write(m_dbData), static_cast<qint64>((m_dbData.size())));
    m_dbFileName = QFileInfo(m_dbFile->fileName()).fileName();
    m_dbFilePath = m_dbFile->fileName();
    m_dbFile->close();

    // make sure window is activated or focus tests may fail
    m_mainWindow->activateWindow();
    QApplication::processEvents();

    fileDialog()->setNextFileName(m_dbFilePath);
    triggerAction("actionDatabaseOpen");

    auto* databaseOpenWidget = m_tabWidget->currentDatabaseWidget()->findChild<QWidget*>("databaseOpenWidget");
    QVERIFY(databaseOpenWidget);
    auto* editPassword = databaseOpenWidget->findChild<QLineEdit*>("editPassword");
    QVERIFY(editPassword);
    editPassword->setFocus();

    QTest::keyClicks(editPassword, "a");
    QTest::keyClick(editPassword, Qt::Key_Enter);

    m_dbWidget = m_tabWidget->currentDatabaseWidget();
    m_db = m_dbWidget->database();
}

// Every test ends with closing the temp database without saving
void TestGuiBrowser::cleanup()
{
    // DO NOT save the database
    m_db->markAsClean();
    MessageBox::setNextAnswer(MessageBox::No);
    triggerAction("actionDatabaseClose");
    QApplication::processEvents();
    MessageBox::setNextAnswer(MessageBox::NoButton);

    if (m_dbWidget) {
        delete m_dbWidget;
    }

    m_dbFile->remove();
}

void TestGuiBrowser::cleanupTestCase()
{
    m_dbFile->remove();
}

void TestGuiBrowser::testEntrySettings()
{
    // Enable the Browser Integration
    config()->set("Browser/Enabled", true);

    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    entryView->setFocus();
    QVERIFY(entryView->hasFocus());

    // Select the first entry in the database
    QModelIndex entryItem = entryView->model()->index(0, 1);
    Entry* entry = entryView->entryFromIndex(entryItem);
    clickIndex(entryItem, entryView, Qt::LeftButton);

    auto* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");

    // Switch to Properties page and select all rows from the custom data table
    editEntryWidget->setCurrentPage(5);
    auto customDataTableView = editEntryWidget->findChild<QTableView*>("customDataTable");
    QVERIFY(customDataTableView);
    QTest::mouseClick(customDataTableView, Qt::LeftButton);
    QTest::keyClick(customDataTableView, 'a', Qt::ControlModifier);

    // Remove the data
    QCOMPARE(entry->customData()->size(), 2);
    auto* removeButton = editEntryWidget->findChild<QPushButton*>("removeCustomDataButton");
    QVERIFY(removeButton);
    MessageBox::setNextAnswer(MessageBox::Delete);
    QTest::mouseClick(removeButton, Qt::LeftButton);

    // Apply the removal
    auto* editEntryWidgetButtonBox = editEntryWidget->findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(editEntryWidgetButtonBox);
    auto* okButton = editEntryWidgetButtonBox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton);
    QTRY_VERIFY(okButton->isEnabled());
    QTest::mouseClick(okButton, Qt::LeftButton);
    QApplication::processEvents();

    QCOMPARE(entry->customData()->size(), 0);
}

void TestGuiBrowser::testAdditionalURLs()
{
    auto* toolBar = m_mainWindow->findChild<QToolBar*>("toolBar");
    auto* entryView = m_dbWidget->findChild<EntryView*>("entryView");

    entryView->setFocus();
    QVERIFY(entryView->hasFocus());

    // Select the first entry in the database
    QModelIndex entryItem = entryView->model()->index(0, 1);
    clickIndex(entryItem, entryView, Qt::LeftButton);

    auto* entryEditAction = m_mainWindow->findChild<QAction*>("actionEntryEdit");
    QWidget* entryEditWidget = toolBar->widgetForAction(entryEditAction);
    QTest::mouseClick(entryEditWidget, Qt::LeftButton);
    QCOMPARE(m_dbWidget->currentMode(), DatabaseWidget::Mode::EditMode);
    auto* editEntryWidget = m_dbWidget->findChild<EditEntryWidget*>("editEntryWidget");

    // Switch to Browser Integration page and add three URL's
    editEntryWidget->setCurrentPage(4);
    auto* addURLButton = editEntryWidget->findChild<QPushButton*>("addURLButton");
    QVERIFY(addURLButton);

    auto* urlList = editEntryWidget->findChild<QListView*>("additionalURLsView");
    QVERIFY(urlList);

    QStringList testURLs = {"https://example1.com", "https://example2.com", "https://example3.com"};

    for (const auto& url : testURLs) {
        QTest::mouseClick(addURLButton, Qt::LeftButton);
        QApplication::processEvents();
        QTest::keyClicks(urlList->focusWidget(), url);
        QTest::keyClick(urlList->focusWidget(), Qt::Key_Enter);
    }

    // Check the values from attributesEdit
    editEntryWidget->setCurrentPage(1);
    auto* attributesView = editEntryWidget->findChild<QListView*>("attributesView");
    auto* attrTextEdit = editEntryWidget->findChild<QPlainTextEdit*>("attributesEdit");

    // Go top of the list
    attributesView->setFocus();
    QTest::keyClick(attributesView->focusWidget(), Qt::Key_PageUp);

    for (const auto& url : testURLs) {
        QCOMPARE(attrTextEdit->toPlainText(), url);
        QTest::keyClick(attributesView->focusWidget(), Qt::Key_Down);
    }
}

void TestGuiBrowser::triggerAction(const QString& name)
{
    auto* action = m_mainWindow->findChild<QAction*>(name);
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
    QApplication::processEvents();
}

void TestGuiBrowser::clickIndex(const QModelIndex& index,
                                QAbstractItemView* view,
                                Qt::MouseButton button,
                                Qt::KeyboardModifiers stateKey)
{
    QTest::mouseClick(view->viewport(), button, stateKey, view->visualRect(index).center());
}
