/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickView>
#include <QHash>

#include "declarativetabmodel.h"
#include "dbmanager.h"
#include "testobject.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   width: 100; height: 100\n" \
        "   property alias tabModel: model\n" \
        "   TabModel { id: model }\n" \
        "}\n";

class tst_declarativetabmodel : public TestObject
{
    Q_OBJECT

public:
    tst_declarativetabmodel();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void validTabs_data();
    void validTabs();

    void activateTabs();

    void remove();
    void closeActiveTab();

    // Navigate forward and back (check url, title changes)
    void forwardBackwardNavigation();
    void multipleTabsWithSameUrls();

    void updateInvalidUrls_data();
    void updateInvalidUrls();

    void updateValidUrls_data();
    void updateValidUrls();

    void invalidTabs_data();
    void invalidTabs();

    void updateTitle();

    void reloadModel();
    void changeTabAndLoad();

    void newTabData();
    void resetNewTabData();

    void clear();

private:
    QStringList modelToStringList(const DeclarativeTabModel *tabModel) const;
    void goBack();
    void goForward();

    bool canGoBack();
    bool canGoForward();

    int currentTabId();

    DeclarativeTabModel *tabModel;
    QList<TestTab> originalTabOrder;
};

tst_declarativetabmodel::tst_declarativetabmodel()
    : TestObject(QML_SNIPPET)
{
    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    originalTabOrder.append(TestTab("http://sailfishos.org", "SailfishOS.org"));
    originalTabOrder.append(TestTab("file:///opt/tests/sailfish-browser/manual/testpage.html", "Test Page"));
    originalTabOrder.append(TestTab("https://sailfishos.org/sailfish-silica/index.html", "Creating applications with Sailfish Silica | Sailfish Silica 1.0"));
    originalTabOrder.append(TestTab("http://www.jolla.com", "Jolla -- we are unlike!"));
}

void tst_declarativetabmodel::initTestCase()
{
    QVERIFY(tabModel);

    if (!tabModel->loaded()) {
        QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
        // Tabs must be loaded with in 500ms
        QVERIFY(loadedSpy.wait());
        QCOMPARE(loadedSpy.count(), 1);
    }
}

void tst_declarativetabmodel::cleanupTestCase()
{
    tabModel->clear();
    QVERIFY(tabModel->count() == 0);

    // Wait for event loop of db manager
    QTest::qWait(500);
    QString dbFileName = QString("%1/%2")
            .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(dbFileName);
    QVERIFY(dbFile.remove());
}

void tst_declarativetabmodel::validTabs_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::addColumn<int>("count");

    for (int i = 0; i < originalTabOrder.count(); ++i) {
        const char *newName = QString("%1-tab").arg(i+1).toLatin1().constData();
        QTest::newRow(newName) << originalTabOrder.at(i).url << originalTabOrder.at(i).title << i+1;
    }
}

void tst_declarativetabmodel::validTabs()
{
    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));

    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(int, count);

    QString previousActiveUrl = tabModel->activeTab().url();

    tabModel->addTab(url, title);
    QCOMPARE(tabModel->count(), count);
    QCOMPARE(countChangeSpy.count(), 1);
    QCOMPARE(tabModel->activeTab().url(), url);
    QCOMPARE(tabModel->activeTab().title(), title);

    if (tabModel->rowCount() > 0) {
        QModelIndex index = tabModel->createIndex(0, 0);
        QCOMPARE(tabModel->data(index, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);
    }
}

void tst_declarativetabmodel::activateTabs()
{
    // Original tab order is now reversed.
    QCOMPARE(tabModel->rowCount(), originalTabOrder.count() - 1);

    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(3).url);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(1).url);
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(0).url);

    // Active: "http://www.jolla.com" (3) -- original index in brachets
    // "https://sailfishos.org/sailfish-silica/index.html" (2)
    // "file:///opt/tests/sailfish-browser/manual/testpage.html" (1)
    // "http://sailfishos.org" (0)
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    for (int i = 0; i < originalTabOrder.count() - 1; ++i) {
        QString previousActiveUrl = tabModel->activeTab().url();
        // Activate always last tab
        tabModel->activateTab(2);
        QCOMPARE(activeTabChangedSpy.count(), 1);
        QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(i).url);
        // Previous active url should be pushed to the first model data index
        QModelIndex modelIndex = tabModel->createIndex(0, 0);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);
        activeTabChangedSpy.clear();
    }

    QString previousActiveUrl = tabModel->activeTab().url();
    // Activate by url
    tabModel->activateTab(originalTabOrder.at(3).url);
    QCOMPARE(activeTabChangedSpy.count(), 1);
    // Previous active url should be pushed to the first model data index
    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);

    // Full loop
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(3).url);
    currentOrder = modelToStringList(tabModel);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(1).url);
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(0).url);
}

void tst_declarativetabmodel::remove()
{
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));
    QCOMPARE(tabModel->count(), originalTabOrder.count());
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(3).url);
    tabModel->remove(1);

    QCOMPARE(activeTabChangedSpy.count(), 0);
    QCOMPARE(tabCountSpy.count(), 1);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabModel->rowCount(), 2);
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(3).url);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(0).url);
}

void tst_declarativetabmodel::closeActiveTab()
{
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));

    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();

    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(3).url);
    tabModel->closeActiveTab();

    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(tabCountSpy.count(), 1);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(tabModel->rowCount(), 1);
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(2).url);
    QCOMPARE(tabModel->activeTab().url(), newActiveUrl);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(0).url);
}

void tst_declarativetabmodel::forwardBackwardNavigation()
{
    tabModel->addTab("http://www.foobar.com/page1", "");
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));

    QString url = "http://www.foobar.com/page2";
    tabModel->updateUrl(currentTabId(), true, url, false);
    activeTabChangedSpy.wait();

    QVERIFY(canGoBack());

    goBack();
    activeTabChangedSpy.wait();

    QVERIFY(!canGoBack());
    QVERIFY(canGoForward());

    QTest::qWait(1000);

    goForward();
    activeTabChangedSpy.wait();

    QVERIFY(canGoBack());
    QVERIFY(!canGoForward());

    goBack();
    activeTabChangedSpy.wait();

    QVERIFY(!canGoBack());
    QVERIFY(canGoForward());

    // Mimic load that started from link clicking.
    url = "http://www.foobar.com/page3";
    tabModel->updateUrl(currentTabId(), true, url, false);
    activeTabChangedSpy.wait();

    QVERIFY(canGoBack());
    QVERIFY(!canGoForward());

    url = "http://www.foobar.com/page4";
    tabModel->updateUrl(currentTabId(), true, url, false);
    activeTabChangedSpy.wait();
    QVERIFY(canGoBack());
    QVERIFY(!canGoForward());

    // page1, page3, page4
    // There items in tab history. Navigate twice back.
    goBack();
    activeTabChangedSpy.wait();
    QVERIFY(canGoBack());
    QVERIFY(canGoForward());

    // Back to first page.
    goBack();
    activeTabChangedSpy.wait();
    QVERIFY(!canGoBack());
    QVERIFY(canGoForward());

    // Wait and check that all updates have come already
    QTest::qWait(1000);

    int expectedCount = tabModel->count() - 1;
    tabModel->removeTabById(currentTabId(), true);
    QVERIFY(tabModel->count() == expectedCount);
    activeTabChangedSpy.wait();
}

void tst_declarativetabmodel::multipleTabsWithSameUrls()
{
    QSignalSpy tabUpdatedFromDb(DBManager::instance(), SIGNAL(tabChanged(Tab)));

    QString page1Tab1Url = "http://www.foobar.com/page1";
    QString page1Tab1Title = "First Page";
    // tab1: page1 ("First Page") and page2 ("")
    tabModel->addTab(page1Tab1Url, page1Tab1Title);
    int tab1 = currentTabId();
    QCOMPARE(tabModel->activeTab().url(), page1Tab1Url);
    QCOMPARE(tabModel->activeTab().title(), page1Tab1Title);

    QString page2Tab1Url = "http://www.foobar.com/page2";
    tabModel->updateUrl(tab1, true, page2Tab1Url, false);
    tabUpdatedFromDb.wait();
    QCOMPARE(tabModel->activeTab().url(), page2Tab1Url);
    // This is a bit problematic. From model point of view only url has changed.
    // In real life between url change and title change there is a short moment
    // when a wrong title / url can slip into the model. Title changes only
    // after engine report the title.
    QVERIFY(tabModel->activeTab().title().isEmpty());

    // tab2: page1 ("First Page Too") and page2 ("Second Page Too")
    QString page1Tab2Url = page1Tab1Url;
    QString page1Tab2Title = "First Page Too";
    tabModel->addTab(page1Tab2Url, page1Tab2Title);
    int tab2 = currentTabId();
    QCOMPARE(tabModel->activeTab().url(), page1Tab2Url);
    QCOMPARE(tabModel->activeTab().title(), page1Tab2Title);

    QString page2Tab2Url = page2Tab1Url;
    QString page2Tab2Title = "Second Page Too";
    tabModel->updateUrl(tab2, true, page2Tab2Url, false);
    tabUpdatedFromDb.wait();
    QCOMPARE(tabModel->activeTab().url(), page2Tab2Url);
    QVERIFY(tabModel->activeTab().title().isEmpty());

    tabModel->updateTitle(tab2, true, page2Tab2Title);
    QCOMPARE(tabModel->activeTab().title(), page2Tab2Title);

    // tab2: go back to page1 ("First Page Too")
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    QVERIFY(canGoBack());
    QVERIFY(!canGoForward());
    goBack();
    activeTabChangedSpy.wait();
    QCOMPARE(tabModel->activeTab().url(), page1Tab2Url);
    QCOMPARE(tabModel->activeTab().title(), page1Tab2Title);
    QVERIFY(!canGoBack());
    QVERIFY(canGoForward());

    // tab1: go back to page1 ("First Page")
    tabModel->activateTabById(tab1);
    // Model has up-to-date data, no need to wait anything from database
    QCOMPARE(tabModel->activeTab().url(), page2Tab1Url);
    QVERIFY(tabModel->activeTab().title().isEmpty());
    QVERIFY(canGoBack());
    QVERIFY(!canGoForward());
    goBack();
    activeTabChangedSpy.wait();

    QCOMPARE(tabModel->activeTab().url(), page1Tab1Url);
    QCOMPARE(tabModel->activeTab().title(), page1Tab1Title);
    QVERIFY(!canGoBack());
    QVERIFY(canGoForward());

    int expectedCount = tabModel->count() - 2;
    tabModel->removeTabById(tab1, true);
    activeTabChangedSpy.wait();
    tabModel->removeTabById(tab2, true);
    activeTabChangedSpy.wait();
    QVERIFY(tabModel->count() == expectedCount);
}

void tst_declarativetabmodel::updateInvalidUrls_data()
{
    QString expectedUrl = tabModel->activeTab().url();
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expectedUrl");
    QTest::newRow("tel") << "tel:+123456798" << expectedUrl;
    QTest::newRow("sms") << "sms:+123456798" << expectedUrl;
    QTest::newRow("mailto") << "mailto:joe@example.com" << expectedUrl;
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1" << expectedUrl;
    QTest::newRow("geo") << "geo:61.49464,23.77513" << expectedUrl;
    QTest::newRow("geo://") << "geo://61.49464,23.77513" << expectedUrl;
}

void tst_declarativetabmodel::updateInvalidUrls()
{
    QFETCH(QString, expectedUrl);
    QFETCH(QString, url);
    tabModel->updateUrl(currentTabId(), true, url, false);
    QCOMPARE(tabModel->activeTab().url(), expectedUrl);
}

void tst_declarativetabmodel::updateValidUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("http") << "http://foobar";
    QTest::newRow("https") << "https://foobar";
    QTest::newRow("file") << "file://foo/bar/index.html";
    QTest::newRow("relative") << "foo/bar/index.html";
}

void tst_declarativetabmodel::updateValidUrls()
{
    QFETCH(QString, url);

    int tabId = currentTabId();
    QSignalSpy tabChangeSpy(DBManager::instance(), SIGNAL(tabChanged(Tab)));
    tabModel->updateUrl(tabId, true, url, false);
    waitSignals(tabChangeSpy, 1);
    QCOMPARE(tabModel->activeTab().url(), url);
}

void tst_declarativetabmodel::invalidTabs_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::newRow("tel") << "tel:+123456798" << "tel";
    QTest::newRow("sms") << "sms:+123456798" << "sms";
    QTest::newRow("mailto") << "mailto:joe@example.com" << "mailto1";
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1" << "mailto2";
}

void tst_declarativetabmodel::invalidTabs()
{
    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));

    QFETCH(QString, url);
    QFETCH(QString, title);
    int originalCount = tabModel->count();
    tabModel->addTab(url, title);

    QCOMPARE(tabModel->count(), originalCount);
    QCOMPARE(countChangeSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);
}

void tst_declarativetabmodel::updateTitle()
{
    QSignalSpy titleChangeSpy(DBManager::instance(), SIGNAL(titleChanged(int,int,QString,QString)));
    int tabId = currentTabId();
    QString title = "A title something";
    tabModel->updateTitle(tabId, true, title);
    waitSignals(titleChangeSpy, 1);

    QCOMPARE(tabModel->activeTab().title(), title);

    QString url = "http://foobar";
    tabModel->addTab(url, "");
    int tab1 = currentTabId();
    QVERIFY(tabModel->activeTab().title().isEmpty());
    QCOMPARE(tabModel->activeTab().url(), url);

    title = "FooBar Title";
    tabModel->updateTitle(tab1, true, title);
    waitSignals(titleChangeSpy, 2);
    QCOMPARE(tabModel->activeTab().title(), title);

    tabModel->updateTitle(tab1, true, "");
    waitSignals(titleChangeSpy, 3);
    QVERIFY(tabModel->activeTab().title().isEmpty());

    // Add a new tab with same url and change title "" -> "FooBar"
    title = "FooBar";
    tabModel->addTab(url, title);
    int tab2 = currentTabId();
    QCOMPARE(tab2, currentTabId());
    QCOMPARE(tabModel->activeTab().title(), title);
    QCOMPARE(tabModel->activeTab().url(), url);

    // Tab1 in model should not be changed after tab added.
    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), url);
    QVERIFY(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString().isEmpty());

    title = "FooBar Two";
    tabModel->updateTitle(tab2, true, title);
    waitSignals(titleChangeSpy, 4);
    QCOMPARE(tabModel->activeTab().title(), title);

    // Tab1 in model should not be changed after title changed for tab2.
    modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), url);
    QVERIFY(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString().isEmpty());

    QString activeTabTitle = tabModel->activeTab().title();

    title = "FooBar non active tab";
    tabModel->updateTitle(tab1, false, title);
    waitSignals(titleChangeSpy, 5);
    QCOMPARE(tabModel->activeTab().title(), activeTabTitle);
    modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), url);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), title);
}

void tst_declarativetabmodel::reloadModel()
{
    setTestData(EMPTY_QML);
    setTestData(QML_SNIPPET);

    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
    QVERIFY(tabModel);
    waitSignals(loadedSpy, 1);

    QCOMPARE(tabModel->count(), 4);
    QCOMPARE(tabModel->activeTab().tabId(), 9);
    QCOMPARE(tabModel->activeTab().currentLink(), 18);
    QCOMPARE(tabModel->activeTab().previousLink(), 0);
    QCOMPARE(tabModel->activeTab().nextLink(), 0);
    QCOMPARE(tabModel->activeTab().url(), QString("http://foobar"));
    QCOMPARE(tabModel->activeTab().title(), QString("FooBar Two"));

    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("http://foobar"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("FooBar non active tab"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 8);
    QCOMPARE(tabModel->m_tabs.at(0).currentLink(), 17);
    QCOMPARE(tabModel->m_tabs.at(0).previousLink(), 0);
    QCOMPARE(tabModel->m_tabs.at(0).nextLink(), 0);

    modelIndex = tabModel->createIndex(1, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("foo/bar/index.html"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("A title something"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 3);
    QCOMPARE(tabModel->m_tabs.at(1).currentLink(), 16);
    QCOMPARE(tabModel->m_tabs.at(1).previousLink(), 15);
    QCOMPARE(tabModel->m_tabs.at(1).nextLink(), 0);

    modelIndex = tabModel->createIndex(2, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("http://sailfishos.org"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("SailfishOS.org"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 1);
    QCOMPARE(tabModel->m_tabs.at(2).currentLink(), 1);
    QCOMPARE(tabModel->m_tabs.at(2).previousLink(), 0);
    QCOMPARE(tabModel->m_tabs.at(2).nextLink(), 0);
}

void tst_declarativetabmodel::changeTabAndLoad()
{
    // Highest linkId of available tabs is 18
    int nextLinkId = DBManager::instance()->nextLinkId();
    QCOMPARE(nextLinkId, 19);

    tabModel->activateTab(1);
    QCOMPARE(currentTabId(), 3);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));

    // Current link becomes previous after url update ("link clicked")
    int previousLink = tabModel->activeTab().currentLink();
    QCOMPARE(previousLink, 16);
    QString url = "http://www.foobar.com/something";
    tabModel->updateUrl(currentTabId(), true, url, false);
    waitSignals(activeTabChangedSpy, 1);

    QCOMPARE(tabModel->activeTab().tabId(), 3);
    QCOMPARE(tabModel->activeTab().currentLink(), nextLinkId);
    QCOMPARE(tabModel->activeTab().previousLink(), previousLink);
    QCOMPARE(tabModel->activeTab().nextLink(), 0);
    QCOMPARE(tabModel->activeTab().url(), url);
    QCOMPARE(tabModel->activeTab().title(), QString(""));
}

void tst_declarativetabmodel::newTabData()
{
    tabModel->resetNewTabData();

    QSignalSpy newTabDataChanged(tabModel, SIGNAL(hasNewTabDataChanged()));
    QSignalSpy newTabUrlChanged(tabModel, SIGNAL(newTabUrlChanged()));
    QVERIFY(tabModel->newTabTitle().isEmpty());
    QObject page;
    tabModel->newTabData("http://foobar.com", "FooBar", &page);

    QCOMPARE(newTabDataChanged.count(), 1);
    QCOMPARE(newTabUrlChanged.count(), 1);

    QCOMPARE(tabModel->newTabUrl(), QString("http://foobar.com"));
    QCOMPARE(tabModel->newTabTitle(), QString("FooBar"));
    QCOMPARE(tabModel->newTabPreviousPage(), &page);
}

void tst_declarativetabmodel::resetNewTabData()
{
    // Old values (see newTabData test):
    // url = "http://foobar.com"
    // title = "FooBar"
    // previousPage = Temporary QObject pointer (non zero)
    QSignalSpy newTabDataChanged(tabModel, SIGNAL(hasNewTabDataChanged()));
    QSignalSpy newTabUrlChanged(tabModel, SIGNAL(newTabUrlChanged()));

    QVERIFY(tabModel->newTabPreviousPage());
    QVERIFY(!tabModel->newTabTitle().isEmpty());
    tabModel->resetNewTabData();

    QCOMPARE(newTabDataChanged.count(), 1);
    QCOMPARE(newTabUrlChanged.count(), 1);

    QVERIFY(!tabModel->hasNewTabData());
    QVERIFY(tabModel->newTabUrl().isEmpty());
    QVERIFY(tabModel->newTabTitle().isEmpty());
    QVERIFY(!tabModel->newTabPreviousPage());
}

void tst_declarativetabmodel::clear()
{
    QVERIFY(tabModel->count() > 0);
    tabModel->clear();
    QVERIFY(tabModel->count() == 0);
}

QStringList tst_declarativetabmodel::modelToStringList(const DeclarativeTabModel *tabModel) const
{
    QStringList list;
    for (int i = 0; i < tabModel->rowCount(); ++i) {
        QModelIndex modelIndex = tabModel->createIndex(i, 0);
        list << tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
    }
    return list;
}

void tst_declarativetabmodel::goBack()
{
    DBManager::instance()->goBack(currentTabId());
}

void tst_declarativetabmodel::goForward()
{
    DBManager::instance()->goForward(currentTabId());
}

bool tst_declarativetabmodel::canGoBack()
{
    return tabModel->activeTab().previousLink() > 0;
}

bool tst_declarativetabmodel::canGoForward()
{
    return tabModel->activeTab().nextLink() > 0;
}

int tst_declarativetabmodel::currentTabId()
{
    if (tabModel->m_activeTab.isValid()) {
        return tabModel->m_activeTab.tabId();
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    tst_declarativetabmodel testcase;
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativetabmodel.moc"
