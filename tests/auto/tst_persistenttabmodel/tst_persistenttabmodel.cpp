
/****************************************************************************
**
** Copyright (c) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest/QtTest>
#include <QFile>
#include <QTemporaryDir>

#include "persistenttabmodel.h"
#include "dbmanager.h"
#include "declarativewebpage.h"
#include "declarativewebcontainer.h"
#include "browserpaths.h"

using ::testing::Return;

class tst_persistenttabmodel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void addTab_data();
    void addTab();
    void newTabInvalidInput_data();
    void newTabInvalidInput();
    void remove();
    void removeTabById_data();
    void removeTabById();
    void clear();
    void activateTabByUrl_data();
    void activateTabByUrl();
    void activateTabById_data();
    void activateTabById();
    void activateTabByIndex_data();
    void activateTabByIndex();
    void closeActiveTab();
    void updateUrl_data();
    void updateUrl();
    void updateThumbnailPath();
    void onUrlChanged();
    void onTitleChanged();
    void nextActiveTabIndex();
    void roleNames();
    void data_data();
    void data();
    void newTabRequested();
    void applyRuntimeSnapshot();
    void runtimeSnapshotThumbnailInvalidation();
    void runtimeDesktopModeIsPerPersistentTab();
    void runtimeAuthoritativeCommands();
    void runtimeRestorePayload();
    void runtimeHistoryTraversal();
    void runtimeHistoryTraversalCancellation();
    void runtimeHistoryTraversalTimeout();
    void runtimeReservationReconciliation();
    void cancelRuntimeTabReservation();
    void runtimeSnapshotRemovalSignalsTabClosed();
    void pendingRuntimeNewTabs();

private:
    void addThreeTabs();

    PersistentTabModel* tabModel;
    QString mDbFile;
};

void tst_persistenttabmodel::initTestCase()
{
    int argc(0);
    char* argv[0] = {};
    ::testing::InitGoogleMock(&argc, argv);
    mDbFile = BrowserPaths::databasePath();
    QFile dbFile(mDbFile);
    dbFile.remove();
}

void tst_persistenttabmodel::init()
{
    int nextTabId = DBManager::instance()->getMaxTabId() + 1;
    tabModel = new PersistentTabModel(nextTabId);
    tabModel->m_unittestMode = true;

    if (!tabModel->loaded()) {
        QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
        // Tabs must be loaded with in 500ms
        QVERIFY(loadedSpy.wait(500));
        QCOMPARE(loadedSpy.count(), 1);
    }
}

void tst_persistenttabmodel::cleanup()
{
    delete tabModel;
    delete DBManager::instance();
    QFile dbFile(mDbFile);
    QVERIFY(dbFile.remove());
}


void tst_persistenttabmodel::addTab_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");
    QTest::addColumn<Tab>("tabToAdd");
    QTest::addColumn<int>("insertToIndex");

    QList<Tab> emptyList;

    QList<Tab> list {
        Tab(1, QString("http://example.com"), QString("Test title1"), QString(), false),
        Tab(2, QString("file:///opt/tests/testpage.html"), QString("Test title2"), QString(), false),
        Tab(3, QString("https://example.com"), QString("Test title3"), QString(), false)
    };
    QTest::newRow("append_to_end") << list << Tab(4, QString("http://example2.com"), QString("Test title4"), QString(), false) << 3;
    QTest::newRow("insert_to_start") << list << Tab(4, QString("http://example2.com"), QString("Test title4"), QString(), false) << 0;
    QTest::newRow("insert_to_empty_model") << emptyList << Tab(1, QString("http://example2.com"), QString("Test title4"), QString(), false) << 0;
}

void tst_persistenttabmodel::addTab()
{
    QFETCH(QList<Tab>, initialTabs);
    QFETCH(Tab, tabToAdd);
    QFETCH(int, insertToIndex);

    // initialize the case
    for (int i = 0; i < initialTabs.count(); i++) {
        tabModel->addTab(initialTabs.at(i), i);
    }

    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));
    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));
    QSignalSpy activeTabIndexChangedSpy(tabModel, SIGNAL(activeTabIndexChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));

    // actual test
    tabModel->addTab(tabToAdd, insertToIndex);

    QCOMPARE(countChangeSpy.count(), 1);

    QCOMPARE(tabAddedSpy.count(), 1);
    QList<QVariant> arguments = tabAddedSpy.at(0);
    QCOMPARE(arguments.at(0).toInt(), initialTabs.count() + 1);

    QCOMPARE(tabModel->nextTabId(), initialTabs.count() + 2);

    QCOMPARE(activeTabChangedSpy.count(), 1);
    arguments = activeTabChangedSpy.at(0);
    QCOMPARE(arguments.at(0).toInt(), initialTabs.count() + 1);

    QCOMPARE(tabModel->activeTab().url(), tabToAdd.url());
    QCOMPARE(tabModel->activeTab().title(), tabToAdd.title());

    // when model is not empty two dataChanged signals are emitted;
    int dataChangedCount = initialTabs.count() == 0 ? 1 : 2;
    QCOMPARE(dataChangedSpy.count(), dataChangedCount);

    QCOMPARE(activeTabIndexChangedSpy.count(), 1);

    QModelIndex modelIndex = tabModel->createIndex(insertToIndex, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), tabToAdd.url());
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), tabToAdd.title());
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), initialTabs.count() + 1);

    if (insertToIndex > 0) {
        modelIndex = tabModel->createIndex(insertToIndex - 1, 0);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), initialTabs.at(insertToIndex - 1).url());
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), initialTabs.at(insertToIndex - 1).title());
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), insertToIndex);
    } else if (insertToIndex < initialTabs.count()) {
        modelIndex = tabModel->createIndex(insertToIndex + 1, 0);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), initialTabs.at(insertToIndex).url());
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), initialTabs.at(insertToIndex).title());
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), insertToIndex + 1);
    }
}

void tst_persistenttabmodel::newTabInvalidInput_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("tel") << "tel:+123456798";
    QTest::newRow("sms") << "sms:+123456798";
    QTest::newRow("mailto") << "mailto:joe@example.com";
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1";
}

void tst_persistenttabmodel::newTabInvalidInput()
{
    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));

    QFETCH(QString, url);
    tabModel->newTab(url, false);

    QCOMPARE(tabModel->count(), 0);
    QCOMPARE(countChangeSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);
}

void tst_persistenttabmodel::remove()
{
    addThreeTabs();

    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));
    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));
    QSignalSpy activeTabIndexChangedSpy(tabModel, SIGNAL(activeTabIndexChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));

    // nothing should happen
    tabModel->remove(-1);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabClosedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 0);
    QCOMPARE(activeTabIndexChangedSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);

    // nothing should happen
    tabModel->remove(10);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabClosedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 0);
    QCOMPARE(activeTabIndexChangedSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);

    // removing inactive tab
    tabModel->remove(1);
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(tabClosedSpy.count(), 1);
    QCOMPARE(activeTabIndexChangedSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 0);

    // remove active tab
    tabModel->remove(tabModel->count() - 1);
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(tabClosedSpy.count(), 2); // by now two tabs have been closed
    QCOMPARE(activeTabIndexChangedSpy.count(), 1);
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.count(), 1);
}

void tst_persistenttabmodel::removeTabById_data()
{
    QTest::addColumn<int>("tabId");
    QTest::addColumn<bool>("activeTab");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("first_tab")       << 1 << false << true;
    QTest::newRow("middle_tab")      << 2 << false << true;
    QTest::newRow("last_tab")        << 3 << false << true;
    QTest::newRow("ignore_tabId_1")  << 1 << true  << true;
    QTest::newRow("ignore_tabId_2")  << 2 << true  << true;
    QTest::newRow("ignore_tabId_3")  << 2 << true  << true;
    QTest::newRow("invalid_tabId_1") << 0 << false << false;
    QTest::newRow("invalid_tabId_2") << 9 << false << false;
    QTest::newRow("invalid_tabId_3") << -1 << false << false;
}

void tst_persistenttabmodel::removeTabById()
{
    addThreeTabs();

    QFETCH(int, tabId);
    QFETCH(bool, activeTab);
    QFETCH(bool, isValid);

    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));

    tabModel->removeTabById(tabId, activeTab);

    if (isValid) {
        QCOMPARE(tabClosedSpy.count(), 1);

        if (activeTab) {
            QCOMPARE(activeTabChangedSpy.count(), 1);
        }
    } else {
        QCOMPARE(tabClosedSpy.count(), 0);
    }
}

void tst_persistenttabmodel::clear()
{
    addThreeTabs();
    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));

    tabModel->clear();

    QCOMPARE(tabClosedSpy.count(), 3);
    QCOMPARE(tabModel->count(), 0);
}

void tst_persistenttabmodel::activateTabByUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<int>("expectedChanges");
    QTest::addColumn<int>("expectedTabId");
    QTest::addColumn<QString>("expectedUrl");
    QTest::addColumn<QString>("expectedTitle");

    QTest::newRow("url_for_inactive_tab") << QString("file:///opt/tests/testpahe.html") << 1 << 2 << QString("file:///opt/tests/testpahe.html") << QString("Test title2");
    QTest::newRow("url_for_active_tab") << QString("https://example.com") << 0 << 3 << QString("https://example.com") << QString("Test title3");
    QTest::newRow("non_existing_url") << QString("http://some.non.existing.url") << 0 << 3 << QString("https://example.com") << QString("Test title3");
}

void tst_persistenttabmodel::activateTabByUrl()
{
    addThreeTabs();

    QFETCH(QString, url);
    QFETCH(int, expectedChanges);
    QFETCH(int, expectedTabId);
    QFETCH(QString, expectedUrl);
    QFETCH(QString, expectedTitle);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));

    tabModel->activateTab(url);

    QCOMPARE(tabModel->activeTabId(), expectedTabId);
    QCOMPARE(tabModel->activeTab().url(), expectedUrl);
    QCOMPARE(tabModel->activeTab().title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), expectedChanges);
}

void tst_persistenttabmodel::activateTabById_data()
{
    QTest::addColumn<int>("tabId");
    QTest::addColumn<int>("expectedChanges");
    QTest::addColumn<int>("expectedTabId");
    QTest::addColumn<QString>("expectedUrl");
    QTest::addColumn<QString>("expectedTitle");

    QTest::newRow("inactive_tab") << 2 << 1 << 2 << QString("file:///opt/tests/testpahe.html") << QString("Test title2");
    QTest::newRow("active_tab") << 3 << 0 << 3 << QString("https://example.com") << QString("Test title3");
    QTest::newRow("out_of_range_1") << -1 << 0 << 3 << QString("https://example.com") << QString("Test title3");
    QTest::newRow("out_of_range_2") << 1000 << 0 << 3 << QString("https://example.com") << QString("Test title3");
}

void tst_persistenttabmodel::activateTabById()
{
    addThreeTabs();

    QFETCH(int, tabId);
    QFETCH(int, expectedChanges);
    QFETCH(int, expectedTabId);
    QFETCH(QString, expectedUrl);
    QFETCH(QString, expectedTitle);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));

    tabModel->activateTabById(tabId);

    QCOMPARE(tabModel->activeTabId(), expectedTabId);
    QCOMPARE(tabModel->activeTab().url(), expectedUrl);
    QCOMPARE(tabModel->activeTab().title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), expectedChanges);
}

void tst_persistenttabmodel::activateTabByIndex_data()
{
    QTest::addColumn<int>("tabIndex");
    QTest::addColumn<int>("expectedChanges");
    QTest::addColumn<int>("expectedTabId");
    QTest::addColumn<QString>("expectedUrl");
    QTest::addColumn<QString>("expectedTitle");

    QTest::newRow("inactive_tab") << 1 << 1 << 2 << QString("file:///opt/tests/testpahe.html") << QString("Test title2");
    QTest::newRow("active_tab") << 2 << 0 << 3 << QString("https://example.com") << QString("Test title3");
    QTest::newRow("out_of_range_1") << -1 << 1 << 1 << QString("http://example.com") << QString("Test title1");
    QTest::newRow("out_of_range_2") << 1000 << 0 << 3 << QString("https://example.com") << QString("Test title3");
}

void tst_persistenttabmodel::activateTabByIndex()
{
    addThreeTabs();

    QFETCH(int, tabIndex);
    QFETCH(int, expectedChanges);
    QFETCH(int, expectedTabId);
    QFETCH(QString, expectedUrl);
    QFETCH(QString, expectedTitle);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));

    tabModel->activateTab(tabIndex);

    QCOMPARE(tabModel->activeTabId(), expectedTabId);
    QCOMPARE(tabModel->activeTab().url(), expectedUrl);
    QCOMPARE(tabModel->activeTab().title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), expectedChanges);
}

void tst_persistenttabmodel::closeActiveTab()
{
    addThreeTabs();
    // make the first tab active
    tabModel->activateTabById(1);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy tabCountChangedSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy activeTabIndexChangedSpy(tabModel, SIGNAL(activeTabIndexChanged()));

    tabModel->closeActiveTab();

    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(tabCountChangedSpy.count(), 1);
    // TODO: this is wrong. Fix DeclarativeTabModel not to emit this signal.
    QCOMPARE(activeTabIndexChangedSpy.count(), 1);
    QCOMPARE(tabModel->activeTabIndex(), 0);
    QCOMPARE(tabModel->activeTabId(), 2);
    QCOMPARE(tabModel->count(), 2);
}

void tst_persistenttabmodel::updateUrl_data()
{
    QTest::addColumn<int>("tabId");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("isExpectedToUpdate");

    QTest::newRow("update_inactive_tab_http") << 1 << "http://some.real.site" << true;
    QTest::newRow("update_inactive_tab_https") << 1 << "https://some.real.site" << true;
    QTest::newRow("update_inactive_tab_file") << 1 << "file:///foo/bar/index.html" << true;
    QTest::newRow("update_inactive_tab_relative") << 1 << "foo/bar/index.html" << true;
    QTest::newRow("update_active_tab") << 3 << "http://some.real.site" << true;
    QTest::newRow("invalid_url_tel") << 3 << "tel:+123456798" << false;
    QTest::newRow("invalid_url_sms") << 3 << "sms:+123456798" << false;
    QTest::newRow("invalid_url_mailto_1") << 3 << "mailto:joe@example.com" << false;
    QTest::newRow("invalid_url_mailto_2") << 3 << "mailto:joe@example.com?cc=bob@example.com&body=hello1" << false;
    QTest::newRow("invalid_url_geo") << 3 << "geo:61.49464,23.77513" << false;
    QTest::newRow("invalid_url_geo://") << 3 << "geo://61.49464,23.77513" << false;
}

void tst_persistenttabmodel::updateUrl()
{
    addThreeTabs();

    QFETCH(int, tabId);
    QFETCH(QString, url);
    QFETCH(bool, isExpectedToUpdate);

    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));

    tabModel->updateUrl(tabId, url);

    if (isExpectedToUpdate) {
        QCOMPARE(dataChangedSpy.count(), 1);
        Tab tab = tabModel->tabs().at(tabModel->findTabIndex(tabId));
        QCOMPARE(tab.url(), url);
    } else {
        QCOMPARE(dataChangedSpy.count(), 0);
    }
}

void tst_persistenttabmodel::updateThumbnailPath()
{
    // set up environment
    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));
    tabModel->addTab(Tab(tabModel->nextTabId(),
                         QLatin1String("http://example.com"),
                         QLatin1String("initial title"), QString(), false), 0);

    QString path("/path/to/thumbnail");
    tabModel->updateThumbnailPath(1, path);
    QCOMPARE(dataChangedSpy.count(), 2);
    QCOMPARE(tabModel->m_tabs.at(0).thumbnailPath(), path);
}

void tst_persistenttabmodel::onUrlChanged()
{
    // set up environment
    tabModel->addTab(Tab(tabModel->nextTabId(),
                         QLatin1String("http://example.com"),
                         QLatin1String("initial title"), QString(), false), 0);

    DeclarativeWebPage mockPage;
    connect(&mockPage, &DeclarativeWebPage::urlChanged, tabModel, &PersistentTabModel::onUrlChanged);

    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));

    // 1. a web page is loaded in an existing tab => update model data
    QUrl url("http://newurl.com");
    EXPECT_CALL(mockPage, tabId()).WillOnce(Return(1));
    EXPECT_CALL(mockPage, url()).WillOnce(Return(url));
    emit mockPage.urlChanged();
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(tabAddedSpy.count(), 0);
}

void tst_persistenttabmodel::onTitleChanged()
{
    // set up environment
    tabModel->addTab(Tab(tabModel->nextTabId(),
                         QLatin1String("http://example.com"),
                         QLatin1String("initial title"), QString(), false), 0);

    DeclarativeWebPage mockPage;
    connect(&mockPage, &DeclarativeWebPage::titleChanged, tabModel, &PersistentTabModel::onTitleChanged);

    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));

    EXPECT_CALL(mockPage, tabId()).WillOnce(Return(1));
    QUrl url;
    EXPECT_CALL(mockPage, url()).WillOnce(Return(url));
    EXPECT_CALL(mockPage, title()).WillOnce(Return(QString("Hello world")));
    emit mockPage.titleChanged();

    QCOMPARE(dataChangedSpy.count(), 1);
}

void tst_persistenttabmodel::nextActiveTabIndex()
{
    DeclarativeWebContainer container;
    DeclarativeWebPage page;
    tabModel->setWebContainer(&container);

    EXPECT_CALL(container, webPage()).WillRepeatedly(Return(&page));
    EXPECT_CALL(page, parentId()).WillOnce(Return(1));
    EXPECT_CALL(page, tabId()).WillOnce(Return(2));
    tabModel->nextActiveTabIndex(0);
}

void tst_persistenttabmodel::roleNames()
{
    // Here we test the method doesn't explode when called.
    tabModel->roleNames();
}

void tst_persistenttabmodel::data_data()
{
    QTest::addColumn<QModelIndex>("index");
    QTest::addColumn<int>("role");
    QTest::addColumn<bool>("isValid");

    QModelIndex modelIndex = tabModel->createIndex(-1, 0);
    QTest::newRow("invalid_index_1") << modelIndex << (int)DeclarativeTabModel::TabIdRole << false;
    modelIndex = tabModel->createIndex(1000, 0);
    QTest::newRow("invalid_index_2") << modelIndex << (int)DeclarativeTabModel::TabIdRole << false;
    modelIndex = tabModel->createIndex(1, 0);
    QTest::newRow("TabIdRole") << modelIndex << (int)DeclarativeTabModel::TabIdRole << true;
    QTest::newRow("ActiveRole") << modelIndex << (int)DeclarativeTabModel::ActiveRole << true;
    QTest::newRow("UrlRole") << modelIndex << (int)DeclarativeTabModel::UrlRole << true;
    QTest::newRow("TitleRole") << modelIndex << (int)DeclarativeTabModel::TitleRole << true;
    QTest::newRow("ThumbPathRole") << modelIndex << (int)DeclarativeTabModel::ThumbPathRole << true;
    QTest::newRow("InvalidRole") << modelIndex << -10000 << false;
}

void tst_persistenttabmodel::data()
{
    // Set up environment
    addThreeTabs();

    QFETCH(QModelIndex, index);
    QFETCH(int, role);
    QFETCH(bool, isValid);

    QVariant data = tabModel->data(index, role);
    if (!isValid) {
        QCOMPARE(data, QVariant());
    } else if (role == DeclarativeTabModel::UrlRole) {
        QCOMPARE(data.toString(), QString("file:///opt/tests/testpahe.html"));
    } else if (role == DeclarativeTabModel::TitleRole) {
        QCOMPARE(data.toString(), QString("Test title2"));
    } else if (role == DeclarativeTabModel::TabIdRole) {
        QCOMPARE(data.toInt(), 2);
    } else if (role == DeclarativeTabModel::ActiveRole) {
        QCOMPARE(data.toBool(), false);
    }
}

void tst_persistenttabmodel::newTabRequested()
{
    QSignalSpy newTabRequestedSpy(tabModel, SIGNAL(newTabRequested(Tab, bool)));
    tabModel->newTab(QLatin1String("http://example.com"), false);
    QCOMPARE(newTabRequestedSpy.count(), 1);
}

void tst_persistenttabmodel::applyRuntimeSnapshot()
{
    QSignalSpy newTabRequestedSpy(tabModel, SIGNAL(newTabRequested(Tab,bool)));
    QSignalSpy adoptedSpy(tabModel, SIGNAL(runtimeTabAdopted(QString,QString)));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy authoritativeActiveSpy(tabModel,
                                      SIGNAL(authoritativeActiveTabChanged(QString)));

    QVariantMap popup;
    popup.insert(QStringLiteral("tabId"), QStringLiteral("100"));
    popup.insert(QStringLiteral("persistentId"), QStringLiteral("0"));
    popup.insert(QStringLiteral("location"), QStringLiteral("https://popup.example/"));
    popup.insert(QStringLiteral("title"), QStringLiteral("Popup"));
    popup.insert(QStringLiteral("discarded"), false);
    tabModel->applyRuntimeSnapshot(QVariantList() << popup, QStringLiteral("100"));

    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(tabModel->activeTabId(), 1);
    QCOMPARE(tabModel->tabs().first().url(), QStringLiteral("https://popup.example/"));
    QCOMPARE(tabModel->tabs().first().title(), QStringLiteral("Popup"));
    QCOMPARE(tabModel->persistentIdForRuntimeId(100), 1);
    QCOMPARE(newTabRequestedSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);
    QCOMPARE(authoritativeActiveSpy.count(), 1);
    QCOMPARE(adoptedSpy.count(), 1);
    QCOMPARE(adoptedSpy.at(0).at(0).toString(), QStringLiteral("100"));
    QCOMPARE(adoptedSpy.at(0).at(1).toString(), QStringLiteral("1"));
    QCOMPARE(tabModel->runtimeIdForPersistentId(QStringLiteral("1")),
             QStringLiteral("100"));
    QCOMPARE(tabModel->persistentIdAt(0), QStringLiteral("1"));

    const QString reservedId = tabModel->reserveRuntimeTab(
                QStringLiteral("https://second.example/"), QStringLiteral("Second"));
    QCOMPARE(reservedId, QStringLiteral("2"));

    QVariantMap second;
    second.insert(QStringLiteral("tabId"), QStringLiteral("200"));
    second.insert(QStringLiteral("persistentId"), reservedId);
    second.insert(QStringLiteral("location"), QStringLiteral("https://second.example/"));
    second.insert(QStringLiteral("title"), QStringLiteral("Second"));
    second.insert(QStringLiteral("discarded"), true);
    popup.insert(QStringLiteral("persistentId"), QStringLiteral("1"));
    popup.insert(QStringLiteral("location"), QStringLiteral("https://popup.example/post"));
    popup.insert(QStringLiteral("title"), QStringLiteral("Post"));
    tabModel->applyRuntimeSnapshot(QVariantList() << second << popup,
                                   QStringLiteral("200"));

    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(tabModel->tabs().at(0).tabId(), 2);
    QCOMPARE(tabModel->tabs().at(1).tabId(), 1);
    QCOMPARE(tabModel->tabs().at(1).url(), QStringLiteral("https://popup.example/post"));
    QCOMPARE(tabModel->tabs().at(1).title(), QStringLiteral("Post"));
    QCOMPARE(tabModel->activeTabId(), 2);
    QCOMPARE(newTabRequestedSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);
    QCOMPARE(adoptedSpy.count(), 1);

    QSignalSpy restoreSpy(DBManager::instance(),
                          SIGNAL(persistentTabRestoreBatchAvailable(PersistentTabRestoreBatch)));
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreBatch batch = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>();
    QCOMPARE(batch.tabs().count(), 2);
    QCOMPARE(batch.tabs().at(0).persistentId(), 2);
    QCOMPARE(batch.tabs().at(1).persistentId(), 1);
    QCOMPARE(batch.activePersistentId(), 2);
}

void tst_persistenttabmodel::runtimeSnapshotThumbnailInvalidation()
{
    QVariantMap runtimeTab;
    runtimeTab.insert(QStringLiteral("tabId"), QStringLiteral("100"));
    runtimeTab.insert(QStringLiteral("persistentId"), QStringLiteral("1"));
    runtimeTab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/one"));
    runtimeTab.insert(QStringLiteral("title"), QStringLiteral("One"));
    runtimeTab.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    tabModel->applyRuntimeSnapshot(QVariantList() << runtimeTab, QStringLiteral("100"));

    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const QString thumbnail = temporaryDir.path() + QStringLiteral("/thumbnail.jpg");
    QFile thumbnailFile(thumbnail);
    QVERIFY(thumbnailFile.open(QIODevice::WriteOnly));
    thumbnailFile.close();
    tabModel->updateThumbnailPath(1, thumbnail);

    runtimeTab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/two"));
    runtimeTab.insert(QStringLiteral("title"), QStringLiteral("Two"));
    runtimeTab.insert(QStringLiteral("locationRevision"), QStringLiteral("2"));
    tabModel->applyRuntimeSnapshot(QVariantList() << runtimeTab, QStringLiteral("100"));

    QCOMPARE(tabModel->tabs().first().thumbnailPath(), QString());
    QVERIFY(!QFile::exists(thumbnail));

    const QString sameLocationThumbnail = temporaryDir.path()
            + QStringLiteral("/same-location-thumbnail.jpg");
    QFile sameLocationThumbnailFile(sameLocationThumbnail);
    QVERIFY(sameLocationThumbnailFile.open(QIODevice::WriteOnly));
    sameLocationThumbnailFile.close();
    tabModel->updateThumbnailPath(1, sameLocationThumbnail);

    runtimeTab.insert(QStringLiteral("locationRevision"), QStringLiteral("3"));
    tabModel->applyRuntimeSnapshot(QVariantList() << runtimeTab,
                                   QStringLiteral("100"));

    QCOMPARE(tabModel->tabs().first().thumbnailPath(), sameLocationThumbnail);
    QVERIFY(QFile::exists(sameLocationThumbnail));
}

void tst_persistenttabmodel::runtimeDesktopModeIsPerPersistentTab()
{
    QVariantMap first;
    first.insert(QStringLiteral("tabId"), QStringLiteral("100"));
    first.insert(QStringLiteral("persistentId"), QStringLiteral("1"));
    first.insert(QStringLiteral("location"), QStringLiteral("https://first.example/"));
    first.insert(QStringLiteral("title"), QStringLiteral("First"));
    first.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    QVariantMap second;
    second.insert(QStringLiteral("tabId"), QStringLiteral("200"));
    second.insert(QStringLiteral("persistentId"), QStringLiteral("2"));
    second.insert(QStringLiteral("location"), QStringLiteral("https://second.example/"));
    second.insert(QStringLiteral("title"), QStringLiteral("Second"));
    second.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    tabModel->applyRuntimeSnapshot(QVariantList() << first << second,
                                   QStringLiteral("100"));

    QSignalSpy dataChangedSpy(tabModel, &QAbstractItemModel::dataChanged);
    QVERIFY(tabModel->setRuntimeDesktopMode(QStringLiteral("2"), true));
    QVERIFY(!tabModel->runtimeDesktopMode(QStringLiteral("1")));
    QVERIFY(tabModel->runtimeDesktopMode(QStringLiteral("2")));
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(DBManager::instance()->getSetting(QStringLiteral("desktopModeTabs")),
             QStringLiteral("2"));

    first.insert(QStringLiteral("locationRevision"), QStringLiteral("2"));
    second.insert(QStringLiteral("locationRevision"), QStringLiteral("2"));
    tabModel->applyRuntimeSnapshot(QVariantList() << first << second,
                                   QStringLiteral("200"));
    QVERIFY(!tabModel->runtimeDesktopMode(QStringLiteral("1")));
    QVERIFY(tabModel->runtimeDesktopMode(QStringLiteral("2")));
}

void tst_persistenttabmodel::runtimeAuthoritativeCommands()
{
    addThreeTabs();
    const int oldActiveTabId = tabModel->activeTabId();
    tabModel->setRuntimeAuthoritative(true);

    QSignalSpy activationSpy(tabModel,
                             SIGNAL(runtimeTabActivationRequested(QString,bool)));
    QSignalSpy closeSpy(tabModel, SIGNAL(runtimeTabCloseRequested(QString)));
    QSignalSpy navigationSpy(tabModel,
                             SIGNAL(runtimeTabNavigationRequested(QString,QString,bool)));
    QSignalSpy clearSpy(tabModel, SIGNAL(runtimeTabsClearRequested()));
    QSignalSpy legacyActivationSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy legacyCloseSpy(tabModel, SIGNAL(tabClosed(int)));

    tabModel->activateTab(0);
    QVERIFY(tabModel->requestRuntimeTabNavigation(
                1, QStringLiteral("https://navigation.example/"), true));
    tabModel->remove(1);
    tabModel->clear();

    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabModel->activeTabId(), oldActiveTabId);
    QCOMPARE(activationSpy.count(), 1);
    QCOMPARE(navigationSpy.count(), 1);
    QCOMPARE(navigationSpy.at(0).at(0).toString(), QStringLiteral("1"));
    QCOMPARE(navigationSpy.at(0).at(1).toString(),
             QStringLiteral("https://navigation.example/"));
    QCOMPARE(navigationSpy.at(0).at(2).toBool(), true);
    QCOMPARE(closeSpy.count(), 1);
    QCOMPARE(clearSpy.count(), 1);
    QCOMPARE(legacyActivationSpy.count(), 0);
    QCOMPARE(legacyCloseSpy.count(), 0);
}

void tst_persistenttabmodel::runtimeRestorePayload()
{
    PersistentTabRestoreData restoreData(7);
    restoreData.setTab(Tab(7, QStringLiteral("https://example.com/two"),
                           QStringLiteral("Two"), QStringLiteral("thumb.png"), false));
    restoreData.addHistoryEntry(PersistentTabHistoryEntry(
                                    QStringLiteral("https://example.com/one"),
                                    QStringLiteral("One")));
    restoreData.addHistoryEntry(PersistentTabHistoryEntry(
                                    QStringLiteral("https://example.com/two"),
                                    QStringLiteral("Two")));
    restoreData.setSelectedHistoryIndex(1);
    PersistentTabRestoreData blankRestoreData(8);
    blankRestoreData.setTab(Tab(8, QString(), QString(), QString(), false));
    tabModel->persistentTabRestoreBatchAvailable(
                PersistentTabRestoreBatch(QList<PersistentTabRestoreData>()
                                          << restoreData << blankRestoreData, 7));

    const QVariantMap batch = tabModel->runtimeRestoreBatch();
    QCOMPARE(batch.value(QStringLiteral("activePersistentId")).toString(),
             QStringLiteral("7"));
    QCOMPARE(batch.value(QStringLiteral("selectedIndex")).toInt(), 0);
    const QVariantList tabs = batch.value(QStringLiteral("tabs")).toList();
    QCOMPARE(tabs.count(), 2);
    const QVariantMap tab = tabs.first().toMap();
    QCOMPARE(tab.value(QStringLiteral("persistentId")).toString(), QStringLiteral("7"));
    QCOMPARE(tab.value(QStringLiteral("selectedHistoryIndex")).toInt(), 1);
    const QVariantList history = tab.value(QStringLiteral("history")).toList();
    QCOMPARE(history.count(), 2);
    QCOMPARE(history.at(0).toMap().value(QStringLiteral("location")).toString(),
             QStringLiteral("https://example.com/one"));
    QCOMPARE(history.at(1).toMap().value(QStringLiteral("title")).toString(),
             QStringLiteral("Two"));
    const QVariantMap blankTab = tabs.at(1).toMap();
    QCOMPARE(blankTab.value(QStringLiteral("persistentId")).toString(),
             QStringLiteral("8"));
    QCOMPARE(blankTab.value(QStringLiteral("selectedHistoryIndex")).toInt(), -1);
    QVERIFY(blankTab.value(QStringLiteral("history")).toList().isEmpty());
}

void tst_persistenttabmodel::runtimeHistoryTraversal()
{
    QVariantMap tab;
    tab.insert(QStringLiteral("tabId"), QStringLiteral("100"));
    tab.insert(QStringLiteral("persistentId"), QStringLiteral("0"));
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/one"));
    tab.insert(QStringLiteral("title"), QStringLiteral("One"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));

    tab.insert(QStringLiteral("persistentId"), QStringLiteral("1"));
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/two"));
    tab.insert(QStringLiteral("title"), QStringLiteral("Two"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("2"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));

    QSignalSpy traversalSpy(tabModel,
                            SIGNAL(runtimeHistoryTraversalConfirmed(QString,QString)));
    QVERIFY(tabModel->runtimeGoBack(QStringLiteral("1")));
    QVERIFY(!tabModel->runtimeGoBack(QStringLiteral("1")));

    // Requesting a runtime traversal only peeks; Gecko remains authoritative
    // until it reports the matching committed location.
    QSignalSpy pendingRestoreSpy(
                DBManager::instance(),
                SIGNAL(persistentTabRestoreBatchAvailable(PersistentTabRestoreBatch)));
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(pendingRestoreSpy.wait(5000));
    QCOMPARE(pendingRestoreSpy.at(0).at(0).value<PersistentTabRestoreBatch>()
             .tabs().first().selectedHistoryIndex(), 1);

    // Loading, progress, and title snapshots do not commit a new location.
    // They must not cancel the pending traversal.
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    tab.insert(QStringLiteral("title"), QStringLiteral("Two updated"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));

    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/one"));
    tab.insert(QStringLiteral("title"), QStringLiteral("One"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("3"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    QCOMPARE(traversalSpy.count(), 1);
    QCOMPARE(traversalSpy.at(0).at(0).toString(), QStringLiteral("100"));
    QCOMPARE(traversalSpy.at(0).at(1).toString(), QStringLiteral("3"));
    QVERIFY(tabModel->consumeConfirmedRuntimeTraversal(QStringLiteral("100"),
                                                       QStringLiteral("3")));
    QVERIFY(!tabModel->consumeConfirmedRuntimeTraversal(QStringLiteral("100"),
                                                        QStringLiteral("3")));

    QSignalSpy restoreSpy(DBManager::instance(),
                          SIGNAL(persistentTabRestoreBatchAvailable(PersistentTabRestoreBatch)));
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreData restored = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>().tabs().first();
    QCOMPARE(restored.history().count(), 2);
    QCOMPARE(restored.selectedHistoryIndex(), 0);

    QVERIFY(tabModel->runtimeGoForward(QStringLiteral("1")));
    QVERIFY(!tabModel->runtimeGoForward(QStringLiteral("1")));
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/two"));
    tab.insert(QStringLiteral("title"), QStringLiteral("Two"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("4"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    QCOMPARE(traversalSpy.count(), 2);
    // An unconsumed confirmation expires on the next complete snapshot.
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    QVERIFY(!tabModel->consumeConfirmedRuntimeTraversal(QStringLiteral("100"),
                                                        QStringLiteral("4")));

    restoreSpy.clear();
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreData forwarded = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>().tabs().first();
    QCOMPARE(forwarded.history().count(), 2);
    QCOMPARE(forwarded.selectedHistoryIndex(), 1);

    // Forward at the boundary cannot leave a marker that suppresses the next
    // real navigation.
    QVERIFY(!tabModel->runtimeGoForward(QStringLiteral("1")));
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/three"));
    tab.insert(QStringLiteral("title"), QStringLiteral("Three"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("5"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    QCOMPARE(traversalSpy.count(), 2);

    restoreSpy.clear();
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreData navigated = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>().tabs().first();
    QCOMPARE(navigated.history().count(), 3);
    QCOMPARE(navigated.selectedHistoryIndex(), 2);
}

void tst_persistenttabmodel::runtimeHistoryTraversalCancellation()
{
    QVariantMap tab;
    tab.insert(QStringLiteral("tabId"), QStringLiteral("100"));
    tab.insert(QStringLiteral("persistentId"), QStringLiteral("0"));
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/one"));
    tab.insert(QStringLiteral("title"), QStringLiteral("One"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));

    tab.insert(QStringLiteral("persistentId"), QStringLiteral("1"));
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/two"));
    tab.insert(QStringLiteral("title"), QStringLiteral("Two"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("2"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));

    QSignalSpy traversalSpy(tabModel,
                            SIGNAL(runtimeHistoryTraversalConfirmed(QString,QString)));
    QVERIFY(tabModel->runtimeGoBack(QStringLiteral("1")));
    QVERIFY(!tabModel->runtimeGoForward(QStringLiteral("1")));

    // Unchanged snapshots and a same-location commit (for example, reload)
    // cannot decide whether Gecko has processed the traversal yet.
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    tab.insert(QStringLiteral("title"), QStringLiteral("Two updated"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("3"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));

    // A different URL without a newer revision is not committed and neither
    // moves nor appends to the persistent history.
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/three"));
    tab.insert(QStringLiteral("title"), QStringLiteral("Three"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    QVERIFY(!tabModel->runtimeGoBack(QStringLiteral("1")));

    QSignalSpy pendingRestoreSpy(
                DBManager::instance(),
                SIGNAL(persistentTabRestoreBatchAvailable(PersistentTabRestoreBatch)));
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(pendingRestoreSpy.wait(5000));
    const PersistentTabRestoreData pendingRestore = pendingRestoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>().tabs().first();
    QCOMPARE(pendingRestore.history().count(), 2);
    QCOMPARE(pendingRestore.selectedHistoryIndex(), 1);

    // A later different committed location rejects the traversal and records
    // a normal navigation from the cursor which never moved.
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("4"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));
    QCOMPARE(traversalSpy.count(), 0);

    QSignalSpy restoreSpy(DBManager::instance(),
                          SIGNAL(persistentTabRestoreBatchAvailable(PersistentTabRestoreBatch)));
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreData restored = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>().tabs().first();
    QCOMPARE(restored.history().count(), 3);
    QCOMPARE(restored.history().at(0).title(), QStringLiteral("One"));
    QCOMPARE(restored.history().at(1).title(), QStringLiteral("Two updated"));
    QCOMPARE(restored.selectedHistoryIndex(), 2);
}

void tst_persistenttabmodel::runtimeHistoryTraversalTimeout()
{
    QVariantMap tab;
    tab.insert(QStringLiteral("tabId"), QStringLiteral("100"));
    tab.insert(QStringLiteral("persistentId"), QStringLiteral("0"));
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/one"));
    tab.insert(QStringLiteral("title"), QStringLiteral("One"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));

    tab.insert(QStringLiteral("persistentId"), QStringLiteral("1"));
    tab.insert(QStringLiteral("location"), QStringLiteral("https://example.com/two"));
    tab.insert(QStringLiteral("title"), QStringLiteral("Two"));
    tab.insert(QStringLiteral("locationRevision"), QStringLiteral("2"));
    tabModel->applyRuntimeSnapshot(QVariantList() << tab, QStringLiteral("100"));

    QVERIFY(tabModel->runtimeGoBack(QStringLiteral("1")));
    QVERIFY(tabModel->m_runtimeTraversalTimer.isActive());
    QVERIFY(!tabModel->runtimeGoBack(QStringLiteral("1")));

    // Simulate an absent Gecko response reaching its bounded deadline.
    tabModel->m_pendingRuntimeTraversals[1].deadline = 0;
    tabModel->expireRuntimeTraversals();
    QVERIFY(!tabModel->m_runtimeTraversalTimer.isActive());

    // The DB cursor was never moved, and a subsequent traversal is accepted.
    QSignalSpy restoreSpy(DBManager::instance(),
                          SIGNAL(persistentTabRestoreBatchAvailable(PersistentTabRestoreBatch)));
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreData restored = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>().tabs().first();
    QCOMPARE(restored.history().count(), 2);
    QCOMPARE(restored.selectedHistoryIndex(), 1);
    QVERIFY(tabModel->runtimeGoBack(QStringLiteral("1")));
}

void tst_persistenttabmodel::runtimeReservationReconciliation()
{
    const QString persistentId = tabModel->reserveRuntimeTab(
                QStringLiteral("https://rejected.example/"), QStringLiteral("Rejected"));
    QCOMPARE(persistentId, QStringLiteral("1"));
    QVERIFY(tabModel->m_runtimeTabReservationTimer.isActive());
    QSignalSpy rejectedSpy(tabModel,
                           SIGNAL(runtimeTabReservationRejected(QString)));

    // The first post-command snapshot is allowed to predate Gecko processing
    // the create request, so the reservation remains durable.
    tabModel->applyRuntimeSnapshot(QVariantList(), QString());
    QSignalSpy restoreSpy(DBManager::instance(),
                          SIGNAL(persistentTabRestoreBatchAvailable(PersistentTabRestoreBatch)));
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreBatch reservedBatch = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>();
    QCOMPARE(reservedBatch.tabs().count(), 1);
    QVERIFY(reservedBatch.tabs().first().history().isEmpty());
    QCOMPARE(reservedBatch.tabs().first().selectedHistoryIndex(), -1);
    QCOMPARE(rejectedSpy.count(), 0);

    // Rejection is bounded even if Gecko never publishes another snapshot.
    tabModel->m_reservedRuntimeTabDeadlines.insert(persistentId.toInt(), 0);
    tabModel->expireRuntimeTabReservations();
    QCOMPARE(rejectedSpy.count(), 1);
    QCOMPARE(rejectedSpy.at(0).at(0).toString(), persistentId);
    restoreSpy.clear();
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    QVERIFY(restoreSpy.at(0).at(0).value<PersistentTabRestoreBatch>().tabs().isEmpty());

    // If Gecko reports the tab after the timeout, its authoritative identity
    // recreates the persistent row rather than losing the successful tab.
    QVariantMap lateTab;
    lateTab.insert(QStringLiteral("tabId"), QStringLiteral("100"));
    lateTab.insert(QStringLiteral("persistentId"), persistentId);
    lateTab.insert(QStringLiteral("location"), QStringLiteral("https://late.example/"));
    lateTab.insert(QStringLiteral("title"), QStringLiteral("Late"));
    lateTab.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    tabModel->applyRuntimeSnapshot(QVariantList() << lateTab, QStringLiteral("100"));
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(tabModel->persistentIdForRuntimeId(100), persistentId.toInt());

    restoreSpy.clear();
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreBatch lateBatch = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>();
    QCOMPARE(lateBatch.tabs().count(), 1);
    QCOMPARE(lateBatch.tabs().first().persistentId(), persistentId.toInt());

    // Authoritative presence before expiry acknowledges a reservation and
    // cancels its timeout.
    const QString acknowledgedId = tabModel->reserveRuntimeTab(
                QStringLiteral("https://acknowledged.example/"),
                QStringLiteral("Acknowledged"));
    QVERIFY(tabModel->m_runtimeTabReservationTimer.isActive());
    QVariantMap acknowledgedTab;
    acknowledgedTab.insert(QStringLiteral("tabId"), QStringLiteral("200"));
    acknowledgedTab.insert(QStringLiteral("persistentId"), acknowledgedId);
    acknowledgedTab.insert(QStringLiteral("location"),
                           QStringLiteral("https://acknowledged.example/"));
    acknowledgedTab.insert(QStringLiteral("title"), QStringLiteral("Acknowledged"));
    acknowledgedTab.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    lateTab.insert(QStringLiteral("persistentId"), persistentId);
    tabModel->applyRuntimeSnapshot(QVariantList() << lateTab << acknowledgedTab,
                                   QStringLiteral("200"));
    QVERIFY(!tabModel->m_reservedRuntimeTabDeadlines.contains(acknowledgedId.toInt()));
    QVERIFY(!tabModel->m_runtimeTabReservationTimer.isActive());
    tabModel->expireRuntimeTabReservations();
    QCOMPARE(rejectedSpy.count(), 1);
}

void tst_persistenttabmodel::cancelRuntimeTabReservation()
{
    addThreeTabs();
    QSignalSpy rejectedSpy(tabModel,
                           SIGNAL(runtimeTabReservationRejected(QString)));

    // An authoritative row is never interpreted as a reservation.
    QVERIFY(!tabModel->cancelRuntimeTabReservation(QStringLiteral("1")));
    QCOMPARE(tabModel->count(), 3);

    const QString reservedId = tabModel->reserveRuntimeTab(
                QStringLiteral("https://cancelled.example/"),
                QStringLiteral("Cancelled"));
    QCOMPARE(reservedId, QStringLiteral("4"));
    QVERIFY(tabModel->cancelRuntimeTabReservation(reservedId));
    QVERIFY(!tabModel->cancelRuntimeTabReservation(reservedId));
    QCOMPARE(rejectedSpy.count(), 1);
    QCOMPARE(rejectedSpy.at(0).at(0).toString(), reservedId);
    QCOMPARE(tabModel->count(), 3);

    QSignalSpy restoreSpy(DBManager::instance(),
                          SIGNAL(persistentTabRestoreBatchAvailable(PersistentTabRestoreBatch)));
    DBManager::instance()->getPersistentTabRestoreBatch();
    QVERIFY(restoreSpy.wait(5000));
    const PersistentTabRestoreBatch batch = restoreSpy.at(0).at(0)
            .value<PersistentTabRestoreBatch>();
    QCOMPARE(batch.tabs().count(), 3);
    for (const PersistentTabRestoreData &tab : batch.tabs()) {
        QVERIFY(tab.persistentId() != reservedId.toInt());
    }
}

void tst_persistenttabmodel::runtimeSnapshotRemovalSignalsTabClosed()
{
    QVariantMap first;
    first.insert(QStringLiteral("tabId"), QStringLiteral("100"));
    first.insert(QStringLiteral("persistentId"), QStringLiteral("0"));
    first.insert(QStringLiteral("location"), QStringLiteral("https://one.example/"));
    first.insert(QStringLiteral("title"), QStringLiteral("One"));
    first.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    tabModel->applyRuntimeSnapshot(QVariantList() << first, QStringLiteral("100"));

    QVariantMap second;
    second.insert(QStringLiteral("tabId"), QStringLiteral("200"));
    second.insert(QStringLiteral("persistentId"), QStringLiteral("0"));
    second.insert(QStringLiteral("location"), QStringLiteral("https://two.example/"));
    second.insert(QStringLiteral("title"), QStringLiteral("Two"));
    second.insert(QStringLiteral("locationRevision"), QStringLiteral("1"));
    first.insert(QStringLiteral("persistentId"), QStringLiteral("1"));
    tabModel->applyRuntimeSnapshot(QVariantList() << first << second,
                                   QStringLiteral("100"));

    QSignalSpy closedSpy(tabModel, SIGNAL(tabClosed(int)));
    second.insert(QStringLiteral("persistentId"), QStringLiteral("2"));
    tabModel->applyRuntimeSnapshot(QVariantList() << second, QStringLiteral("200"));
    QCOMPARE(closedSpy.count(), 1);
    QCOMPARE(closedSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(tabModel->persistentIdForRuntimeId(100), 0);

    // An unchanged authoritative snapshot does not repeat close semantics.
    tabModel->applyRuntimeSnapshot(QVariantList() << second, QStringLiteral("200"));
    QCOMPARE(closedSpy.count(), 1);
}

void tst_persistenttabmodel::pendingRuntimeNewTabs()
{
    tabModel->setRuntimeAuthoritative(true);

    const int firstId = tabModel->newTab(
                QStringLiteral("https://first.example/"), true);
    const int secondId = tabModel->newTab(
                QStringLiteral("https://second.example/"), false);
    QCOMPARE(firstId, 1);
    QCOMPARE(secondId, 2);

    const QVariantList pendingCommands = tabModel->takePendingRuntimeNewTabs();
    QCOMPARE(pendingCommands.count(), 2);
    QCOMPARE(pendingCommands.at(0).toMap().value(QStringLiteral("url")).toString(),
             QStringLiteral("https://first.example/"));
    QCOMPARE(pendingCommands.at(0).toMap()
             .value(QStringLiteral("persistentId")).toString(), QStringLiteral("1"));
    QCOMPARE(pendingCommands.at(0).toMap()
             .value(QStringLiteral("fromExternal")).toBool(), true);
    QCOMPARE(pendingCommands.at(1).toMap().value(QStringLiteral("url")).toString(),
             QStringLiteral("https://second.example/"));
    QCOMPARE(pendingCommands.at(1).toMap()
             .value(QStringLiteral("persistentId")).toString(), QStringLiteral("2"));
    QCOMPARE(pendingCommands.at(1).toMap()
             .value(QStringLiteral("fromExternal")).toBool(), false);
    QVERIFY(tabModel->takePendingRuntimeNewTabs().isEmpty());

    const int cancelledId = tabModel->newTab(
                QStringLiteral("https://cancelled.example/"), false);
    QVERIFY(tabModel->cancelRuntimeTabReservation(QString::number(cancelledId)));
    QVERIFY(tabModel->takePendingRuntimeNewTabs().isEmpty());

    const int expiredId = tabModel->newTab(
                QStringLiteral("https://expired.example/"), false);
    tabModel->m_reservedRuntimeTabDeadlines.insert(expiredId, 0);
    tabModel->expireRuntimeTabReservations();
    QVERIFY(tabModel->takePendingRuntimeNewTabs().isEmpty());
}

void tst_persistenttabmodel::addThreeTabs()
{
    QList<QString> urls, titles;
    urls << "http://example.com" << "file:///opt/tests/testpahe.html" << "https://example.com";
    titles << "Test title1" << "Test title2" << "Test title3";

    for (int i = 0; i < urls.count(); i++) {
        tabModel->addTab(Tab(tabModel->nextTabId(), urls.at(i), titles.at(i), QString(), false), tabModel->count());
    }
}

QTEST_MAIN(tst_persistenttabmodel)
#include "tst_persistenttabmodel.moc"
