/****************************************************************************
**
** Copyright (c) 2014 - 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>
#include "dbmanager.h"
#include "browserpaths.h"

Q_DECLARE_METATYPE(QList<Tab>)
Q_DECLARE_METATYPE(QList<Link>)

class tst_dbmanager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void createTab();
    void getAllTabs_data();
    void getAllTabs();
    void removeTab_data();
    void removeTab();
    void removeAllTabs_data();
    void removeAllTabs();
    void clearHistory_data();
    void clearHistory();
    void requestAndResolveUrl_data();
    void requestAndResolveUrl();
    void navigateTo();
    void goBack();
    void goForward();
    void updateThumbPath();
    void updateTitle();
    void getHistory();
    void getTabHistory();
    void saveSetting();
    void deleteSetting();
    void getMaxTabId();

private:
    QString mDbFile;
};

void tst_dbmanager::initTestCase()
{
    mDbFile = QString("%1/%2")
            .arg(BrowserPaths::dataLocation())
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(mDbFile);
    dbFile.remove();
}

void tst_dbmanager::cleanup()
{
    delete DBManager::instance();
    QFile dbFile(mDbFile);
    QVERIFY(dbFile.remove());
}

void tst_dbmanager::createTab()
{
    const Tab tab(1, "http://example.com", "Test title", "", false);
    DBManager::instance()->createTab(tab);

    // Check that we have really inserted the tab.
    // Since all DB operations go through a single thread we don't
    // have to wait until DBWorker completes tab creation.
    QSignalSpy tabsAvailableSpy(DBManager::instance(),
                                SIGNAL(tabsAvailable(QList<Tab>)));
    DBManager::instance()->getAllTabs();
    QVERIFY(tabsAvailableSpy.wait(5000));
    QCOMPARE(tabsAvailableSpy.count(), 1);
    QList<QVariant> arguments = tabsAvailableSpy.at(0);
    Tab newTab = arguments.at(0).value<QList<Tab> >().at(0);
    QCOMPARE(newTab.url(), tab.url());
    QCOMPARE(newTab.tabId(), 1);

    // create one more identical tab
    DBManager::instance()->createTab(Tab(2, "http://example.com", "Test title", "", false));
    DBManager::instance()->getAllTabs();
    QVERIFY(tabsAvailableSpy.wait(5000));
    QCOMPARE(tabsAvailableSpy.count(), 2);
    arguments = tabsAvailableSpy.at(1);
    QCOMPARE(arguments.at(0).value<QList<Tab> >().count(), 2);

    // and one more identical tab, but with empty title
    DBManager::instance()->createTab(Tab(3, "http://example.com", "", "", false));
    DBManager::instance()->getAllTabs();
    QVERIFY(tabsAvailableSpy.wait(5000));
    QCOMPARE(tabsAvailableSpy.count(), 3);
    arguments = tabsAvailableSpy.at(2);
    QCOMPARE(arguments.at(0).value<QList<Tab> >().count(), 3);
}

void tst_dbmanager::getAllTabs_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");

    QList<Tab> emptyList;

    QList<Tab> list {
        Tab(1, "http://example1.com", "Test title 1", "", false),
        Tab(2, "http://example2.com", "Test title 2", "", false),
        Tab(3, "http://example3.com", "Test title 3", "", false),
    };

    QTest::newRow("no_inital_tabs") << emptyList;
    QTest::newRow("three_initial_tabs") << list;
}

void tst_dbmanager::getAllTabs()
{
    QFETCH(QList<Tab>, initialTabs);

    // initialize the case
    for (const Tab &tab : initialTabs) {
        DBManager::instance()->createTab(tab);
    }

    QSignalSpy tabsAvailableSpy(DBManager::instance(),
                                SIGNAL(tabsAvailable(QList<Tab>)));

    // actual test
    DBManager::instance()->getAllTabs();
    QVERIFY(tabsAvailableSpy.wait(5000));
    QCOMPARE(tabsAvailableSpy.count(), 1);
    QList<QVariant> arguments = tabsAvailableSpy.at(0);
    QCOMPARE(arguments.at(0).value<QList<Tab> >().count(), initialTabs.count());
}

void tst_dbmanager::removeTab_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");
    QTest::addColumn<int>("tabId");
    QTest::addColumn<int>("expectedTabsAvailable");

    QList<Tab> emptyList;

    QList<Tab> oneTab {
        Tab(1, "http://example1.com", "Test title 1", "", false)
    };

    QList<Tab> threeTabs {
        Tab(1, "http://example1.com", "Test title 1", "", false),
        Tab(2, "http://example2.com", "Test title 2", "", false),
        Tab(3, "http://example3.com", "Test title 3", "", false)
    };

    QTest::newRow("no_inital_tabs") << emptyList << 1 << 1;
    QTest::newRow("one_inital_tab") << oneTab << 1 << 1;
    QTest::newRow("one_inital_tab_but_wrong_tabId") << oneTab << 2 << 0;
    QTest::newRow("three_initial_tabs") << threeTabs << 2 << 0;
    QTest::newRow("three_initial_tabs_but_wrong_tabId") << threeTabs << 5 << 0;
}

void tst_dbmanager::removeTab()
{
    QFETCH(QList<Tab>, initialTabs);
    QFETCH(int, tabId);
    QFETCH(int, expectedTabsAvailable);

    // initialize the case
    for (const Tab &tab : initialTabs) {
        DBManager::instance()->createTab(tab);
    }

    QSignalSpy tabsAvailableSpy(DBManager::instance(),
                                SIGNAL(tabsAvailable(QList<Tab>)));

    // actual test
    DBManager::instance()->removeTab(tabId);
    tabsAvailableSpy.wait(1000);
    QCOMPARE(tabsAvailableSpy.count(), expectedTabsAvailable);
}

void tst_dbmanager::removeAllTabs_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");

    QList<Tab> emptyList;

    QList<Tab> oneTab {
        Tab(1, "http://example1.com", "Test title 1", "", false)
    };

    QList<Tab> threeTabs {
        Tab(1, "http://example1.com", "Test title 1", "", false),
        Tab(2, "http://example2.com", "Test title 2", "", false),
        Tab(3, "http://example3.com", "Test title 3", "", false)
    };

    QTest::newRow("no_inital_tabs") << emptyList;
    QTest::newRow("one_inital_tab") << oneTab;
    QTest::newRow("three_initial_tabs") << threeTabs;
}

void tst_dbmanager::removeAllTabs()
{
    QFETCH(QList<Tab>, initialTabs);

    // initialize the case
    for (const Tab &tab : initialTabs) {
        DBManager::instance()->createTab(tab);
    }

    QSignalSpy tabsAvailableSpy(DBManager::instance(),
                                SIGNAL(tabsAvailable(QList<Tab>)));

    // actual test
    DBManager::instance()->removeAllTabs();
    tabsAvailableSpy.wait(1000);
    // Make sure there is no feedback from dbworker thread
    QCOMPARE(tabsAvailableSpy.count(), 0);
}

void tst_dbmanager::clearHistory_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");
    QTest::addColumn<int>("expectedTabsAvailable");

    QList<Tab> emptyList;

    QList<Tab> oneTab {
        Tab(1, "http://example1.com", "Test title 1", "", false)
    };

    QList<Tab> threeTabs {
        Tab(1, "http://example1.com", "Test title 1", "", false),
        Tab(2, "http://example2.com", "Test title 2", "", false),
        Tab(3, "http://example3.com", "Test title 3", "", false)
    };

    QTest::newRow("no_inital_tabs") << emptyList << 0;
    QTest::newRow("one_inital_tab") << oneTab << 1;
    QTest::newRow("three_initial_tabs") << threeTabs << 1;
}

void tst_dbmanager::clearHistory()
{
    QFETCH(QList<Tab>, initialTabs);
    QFETCH(int, expectedTabsAvailable);

    // initialize the case
    for (const Tab &tab : initialTabs) {
        DBManager::instance()->createTab(tab);
    }

    QSignalSpy tabsAvailableSpy(DBManager::instance(),
                                SIGNAL(tabsAvailable(QList<Tab>)));
    QSignalSpy historyAvailableSpy(DBManager::instance(),
                                   SIGNAL(historyAvailable(QList<Link>)));

    // actual test
    DBManager::instance()->clearHistory();
    QVERIFY(historyAvailableSpy.wait(5000));
    QCOMPARE(historyAvailableSpy.count(), 1);
    QCOMPARE(tabsAvailableSpy.count(), expectedTabsAvailable);
}

void tst_dbmanager::requestAndResolveUrl_data()
{
    QTest::addColumn<QString>("requestedUrl");
    QTest::addColumn<QString>("resolvedUrl");

    QTest::newRow("www_requestedUrl") << "www.example.com" << "https://www.example.com/";
    QTest::newRow("http_requestedUrl") << "http://www.example.com" << "https://www.example.com/";
}

void tst_dbmanager::requestAndResolveUrl()
{
    QFETCH(QString, requestedUrl);
    QFETCH(QString, resolvedUrl);

    DBManager::instance()->createTab(Tab(1, requestedUrl, QString(), QString(), false));

    QSignalSpy historyAvailableSpy(DBManager::instance(),
                                   SIGNAL(historyAvailable(QList<Link>)));

    DBManager::instance()->getHistory();
    QVERIFY(historyAvailableSpy.wait(5000));

    QList<QVariant> arguments = historyAvailableSpy.at(0);
    QList<Link> links = arguments.at(0).value<QList<Link> >();
    QCOMPARE(links.count(), 1);
    QCOMPARE(links.at(0).url(), requestedUrl);

    // Update resolved url
    historyAvailableSpy.clear();
    DBManager::instance()->updateUrl(1, requestedUrl, resolvedUrl);

    // Requested url should be gone and only resolved exist.
    // Requested url matches
    historyAvailableSpy.clear();
    DBManager::instance()->getHistory();
    QVERIFY(historyAvailableSpy.wait(5000));

    arguments = historyAvailableSpy.at(0);
    links = arguments.at(0).value<QList<Link> >();
    QCOMPARE(links.count(), 1);
    QCOMPARE(links.at(0).url(), resolvedUrl);
}

void tst_dbmanager::navigateTo()
{
    Tab tab(1, "http://example1.com", "Test title 1", "", false);
    DBManager::instance()->createTab(tab);

    // 1. Navigate to new URLs
    DBManager::instance()->navigateTo(1, "http://example2.com", "Test title 2", "");
    DBManager::instance()->navigateTo(1, "http://example3.com", "Test title 3", "");

    QSignalSpy tabHistoryAvailableSpy(DBManager::instance(),
                                      SIGNAL(tabHistoryAvailable(int,QList<Link>,int)));
    DBManager::instance()->getTabHistory(1);
    QVERIFY(tabHistoryAvailableSpy.wait(5000));

    QList<QVariant> arguments = tabHistoryAvailableSpy.at(0);
    QList<Link> links = arguments.at(1).value<QList<Link> >();
    int currentLinkId = arguments.at(2).toInt();
    QCOMPARE(links.count(), 3);
    QCOMPARE(currentLinkId, 3);
    QCOMPARE(links.at(0).url(), QString("http://example3.com"));

    // 2. Go back and navigate to another URL. Check the obsolete URL got overriden.
    DBManager::instance()->goBack(1);
    DBManager::instance()->navigateTo(1, "http://example4.com", "Test title 4", "");

    DBManager::instance()->getTabHistory(1);
    QVERIFY(tabHistoryAvailableSpy.wait(5000));

    arguments = tabHistoryAvailableSpy.at(1);
    links = arguments.at(1).value<QList<Link> >();
    currentLinkId = arguments.at(2).toInt();
    QCOMPARE(links.count(), 3);
    QCOMPARE(currentLinkId, 4);
    QCOMPARE(links.at(0).url(), QString("http://example4.com"));

    QSignalSpy historyAvailableSpy(DBManager::instance(),
                                   SIGNAL(historyAvailable(QList<Link>)));
    DBManager::instance()->getHistory("example");

    QVERIFY(historyAvailableSpy.wait(5000));
    arguments = historyAvailableSpy.at(0);
    links = arguments.at(0).value<QList<Link> >();
    QCOMPARE(links.count(), 4);
}

void tst_dbmanager::goBack()
{
    // initialize test case
    Tab tab(1, "http://example1.com", "Test title 1", "", false);
    DBManager::instance()->createTab(tab);
    DBManager::instance()->navigateTo(1, "http://example2.com", "Test title 2", "");
    DBManager::instance()->navigateTo(1, "http://example3.com", "Test title 3", "");

    // actual test
    DBManager::instance()->goBack(1);

    QSignalSpy tabHistoryAvailableSpy(DBManager::instance(),
                                      SIGNAL(tabHistoryAvailable(int,QList<Link>,int)));
    DBManager::instance()->getTabHistory(1);
    QVERIFY(tabHistoryAvailableSpy.wait(5000));

    QList<QVariant> arguments = tabHistoryAvailableSpy.at(0);
    QList<Link> links = arguments.at(1).value<QList<Link> >();
    int currentLinkId = arguments.at(2).toInt();
    QCOMPARE(links.count(), 3);
    QCOMPARE(currentLinkId, 2);
}

void tst_dbmanager::goForward()
{
    // initialize test case
    Tab tab(1, "http://example1.com", "Test title 1", "", false);
    DBManager::instance()->createTab(tab);
    DBManager::instance()->navigateTo(1, "http://example2.com", "Test title 2", "");
    DBManager::instance()->navigateTo(1, "http://example3.com", "Test title 3", "");

    // actual test
    DBManager::instance()->goBack(1);
    DBManager::instance()->goForward(1);

    QSignalSpy tabHistoryAvailableSpy(DBManager::instance(),
                                      SIGNAL(tabHistoryAvailable(int,QList<Link>,int)));
    DBManager::instance()->getTabHistory(1);
    QVERIFY(tabHistoryAvailableSpy.wait(5000));

    QList<QVariant> arguments = tabHistoryAvailableSpy.at(0);
    QList<Link> links = arguments.at(1).value<QList<Link> >();
    int currentLinkId = arguments.at(2).toInt();
    QCOMPARE(links.count(), 3);
    QCOMPARE(currentLinkId, 3);
}

void tst_dbmanager::updateThumbPath()
{
    // initialize test case
    Tab tab(1, "http://example1.com", "Test title 1", "", false);
    DBManager::instance()->createTab(tab);

    QSignalSpy thumbPathChangedSpy(DBManager::instance(),
                                   SIGNAL(thumbPathChanged(int,QString)));
    QString thumbPath("/path/to/thumbnail");

    // actual test
    DBManager::instance()->updateThumbPath(1, thumbPath);

    QVERIFY(thumbPathChangedSpy.wait(5000));
    QList<QVariant> arguments = thumbPathChangedSpy.at(0);
    QCOMPARE(arguments.at(1).toString(), thumbPath);

    // check that thumbnail made its way to DB
    QSignalSpy tabsAvailableSpy(DBManager::instance(),
                                SIGNAL(tabsAvailable(QList<Tab>)));
    DBManager::instance()->getAllTabs();
    QVERIFY(tabsAvailableSpy.wait(5000));
    QCOMPARE(tabsAvailableSpy.count(), 1);
    arguments = tabsAvailableSpy.at(0);
    QCOMPARE(arguments.at(0).value<QList<Tab> >().at(0).thumbnailPath(), thumbPath);
}

void tst_dbmanager::updateTitle()
{
    // initialize test case
    QString url("http://example1.com");
    Tab tab(1, url, "Test title 1", "", false);
    DBManager::instance()->createTab(tab);

    QSignalSpy titleChangedSpy(DBManager::instance(),
                               SIGNAL(titleChanged(QString,QString)));
    QString newTitle("New Test Title 2");

    // actual test
    DBManager::instance()->updateTitle(1, url, newTitle);

    QVERIFY(titleChangedSpy.wait(5000));
    QList<QVariant> arguments = titleChangedSpy.at(0);
    QCOMPARE(arguments.at(1).toString(), newTitle);

    // check that title made its way to DB
    QSignalSpy tabsAvailableSpy(DBManager::instance(),
                                SIGNAL(tabsAvailable(QList<Tab>)));
    DBManager::instance()->getAllTabs();
    QVERIFY(tabsAvailableSpy.wait(5000));
    QCOMPARE(tabsAvailableSpy.count(), 1);
    arguments = tabsAvailableSpy.at(0);
    QCOMPARE(arguments.at(0).value<QList<Tab> >().at(0).title(), newTitle);
}

void tst_dbmanager::getHistory()
{
    // initialize test case
    Tab tab1(1, "http://example1.com", "Test title 1", "", false);
    DBManager::instance()->createTab(tab1);
    DBManager::instance()->navigateTo(1, "http://example3-long.com", "Test title 2", "");
    DBManager::instance()->navigateTo(1, "http://unneeded1.net", "Test title 3", "");
    Tab tab2(2, "http://example2.com", "Test title 4", "", false);
    DBManager::instance()->createTab(tab2);
    Tab tab3(3, "http://unneeded2.net", "Test title 5", "", false);
    DBManager::instance()->createTab(tab3);

    QSignalSpy historyAvailableSpy(DBManager::instance(),
                                   SIGNAL(historyAvailable(QList<Link>)));

    // actual test
    // 1. non empty filter
    DBManager::instance()->getHistory(QString("example"));
    QVERIFY(historyAvailableSpy.wait(5000));
    QCOMPARE(historyAvailableSpy.count(), 1);
    QList<QVariant> arguments = historyAvailableSpy.at(0);
    QList<Link> links = arguments.at(0).value<QList<Link> >();
    QCOMPARE(links.count(), 3);
    QSet<QString> linkset;
    for (Link link : links) {
        linkset.insert(link.url());
    }
    QVERIFY(linkset.contains(QString("http://example1.com")));
    QVERIFY(linkset.contains(QString("http://example2.com")));
    QVERIFY(linkset.contains(QString("http://example3-long.com")));

    // 2. empty filter
    DBManager::instance()->getHistory(QString());
    QVERIFY(historyAvailableSpy.wait(5000));
    QCOMPARE(historyAvailableSpy.count(), 2);
    arguments = historyAvailableSpy.at(1);
    links = arguments.at(0).value<QList<Link> >();
    QCOMPARE(links.count(), 5);
    linkset.clear();
    for (Link link : links) {
        linkset.insert(link.url());
    }
    QVERIFY(linkset.contains(QString("http://example1.com")));
    QVERIFY(linkset.contains(QString("http://example2.com")));
    QVERIFY(linkset.contains(QString("http://example3-long.com")));
    QVERIFY(linkset.contains(QString("http://unneeded1.net")));
    QVERIFY(linkset.contains(QString("http://unneeded2.net")));
}

void tst_dbmanager::getTabHistory()
{
    // initialize test case
    Tab tab(1, "http://example1.com", "Test title 1", "", false);
    DBManager::instance()->createTab(tab);
    DBManager::instance()->navigateTo(1, "http://example2.com", "Test title 2", "");
    DBManager::instance()->navigateTo(1, "http://example3.com", "Test title 3", "");

    QSignalSpy tabHistoryAvailableSpy(DBManager::instance(),
            SIGNAL(tabHistoryAvailable(int,QList<Link>,int)));

    // actual test
    DBManager::instance()->getTabHistory(1);

    QVERIFY(tabHistoryAvailableSpy.wait(5000));
    QList<QVariant> arguments = tabHistoryAvailableSpy.at(0);
    QList<Link> links = arguments.at(1).value<QList<Link> >();
    int currentLinkId = arguments.at(2).toInt();
    QCOMPARE(links.count(), 3);
    QCOMPARE(currentLinkId, 3);
}

void tst_dbmanager::saveSetting()
{
    QSignalSpy settingChangedSpy1(DBManager::instance(), SIGNAL(settingsChanged()));

    // insert new setting
    DBManager::instance()->saveSetting("test_key", "test_value");

    QCOMPARE(settingChangedSpy1.count(), 1);

    // delete to make sure the data is persistent and check
    delete DBManager::instance();
    QCOMPARE(DBManager::instance()->getSetting("test_key"), QString("test_value"));
    QCOMPARE(DBManager::instance()->getSetting("nonexisting_key"), QString(""));

    QSignalSpy settingChangedSpy2(DBManager::instance(), SIGNAL(settingsChanged()));

    // update the setting and check
    DBManager::instance()->saveSetting("test_key", "test_new_value");
    QCOMPARE(DBManager::instance()->getSetting("test_key"), QString("test_new_value"));
    QCOMPARE(settingChangedSpy2.count(), 1);
    delete DBManager::instance();
    QCOMPARE(DBManager::instance()->getSetting("test_key"), QString("test_new_value"));
}

void tst_dbmanager::deleteSetting()
{
    DBManager::instance()->saveSetting("test_key", "test_value");
    QCOMPARE(DBManager::instance()->getSetting("test_key"), QString("test_value"));

    QSignalSpy settingChangedSpy(DBManager::instance(), SIGNAL(settingsChanged()));
    DBManager::instance()->deleteSetting("test_key");
    QCOMPARE(DBManager::instance()->getSetting("test_key"), QString(""));
    QCOMPARE(settingChangedSpy.count(), 1);

    // delete to make sure the data was deleted persistently and check
    delete DBManager::instance();
    QCOMPARE(DBManager::instance()->getSetting("test_key"), QString(""));
}

void tst_dbmanager::getMaxTabId()
{
    QCOMPARE(DBManager::instance()->getMaxTabId(), 0);

    const Tab tab(1, "http://example.com", "Test title", "", false);
    DBManager::instance()->createTab(tab);

    QCOMPARE(DBManager::instance()->getMaxTabId(), 1);
}

QTEST_MAIN(tst_dbmanager)
#include "tst_dbmanager.moc"
