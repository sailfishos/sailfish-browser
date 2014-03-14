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

#include "declarativetab.h"
#include "declarativetabmodel.h"
#include "declarativewebcontainer.h"
#include "dbmanager.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "WebContainer {\n" \
        "   property alias tabModel: model\n" \
        "   width: 100; height: 100\n" \
        "   TabModel { id: model }\n" \
        "}\n";

struct TestTab {
    TestTab(QString url, QString title) : url(url), title(title) {}

    QString url;
    QString title;
};

class tst_declarativetabmodel : public QObject
{
    Q_OBJECT

public:
    tst_declarativetabmodel(QObject *parent = 0);

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

    void clear();

private:
    QStringList modelToStringList(const DeclarativeTabModel *tabModel) const;

    DeclarativeTabModel *tabModel;
    DeclarativeWebContainer *webContainer;
    QQuickView view;
    QList<TestTab> originalTabOrder;
};

tst_declarativetabmodel::tst_declarativetabmodel(QObject *parent)
    : QObject(parent)
    , tabModel(0)
{
    originalTabOrder.append(TestTab("http://sailfishos.org", "SailfishOS.org"));
    originalTabOrder.append(TestTab("file:///opt/tests/sailfish-browser/manual/testpage.html", "Test Page"));
    originalTabOrder.append(TestTab("https://sailfishos.org/sailfish-silica/index.html", "Creating applications with Sailfish Silica | Sailfish Silica 1.0"));
    originalTabOrder.append(TestTab("http://www.jolla.com", "Jolla -- we are unlike!"));
}

void tst_declarativetabmodel::initTestCase()
{
    QQmlComponent component(view.engine());
    component.setData(QML_SNIPPET, QUrl());
    QObject *obj = component.create(view.engine()->rootContext());

    view.setContent(QUrl(""), 0, obj);
    view.show();
    QTest::qWaitForWindowExposed(&view);

    webContainer = qobject_cast<DeclarativeWebContainer *>(obj);
    QVERIFY(webContainer);

    QVariant var = obj->property("tabModel");
    tabModel = qobject_cast<DeclarativeTabModel *>(qvariant_cast<QObject*>(var));
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
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));
    QSignalSpy currentUrlChangedSpy(webContainer->currentTab(), SIGNAL(urlChanged()));

    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(int, count);

    QString previousActiveUrl = webContainer->currentTab()->url();

    tabModel->addTab(url, title);
    QCOMPARE(tabModel->count(), count);
    QCOMPARE(countChangeSpy.count(), 1);
    QCOMPARE(currentTabIdChangeSpy.count(), 1);
    QCOMPARE(currentUrlChangedSpy.count(), 1);
    QCOMPARE(webContainer->currentTab()->url(), url);
    QCOMPARE(webContainer->currentTab()->title(), title);

    if (tabModel->rowCount() > 0) {
        QModelIndex index = tabModel->createIndex(0, 0);
        QCOMPARE(tabModel->data(index, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);
    }
}

void tst_declarativetabmodel::activateTabs()
{
    // Original tab order is now reversed.
    QCOMPARE(tabModel->rowCount(), originalTabOrder.count() - 1);

    QCOMPARE(webContainer->currentTab()->url(), originalTabOrder.at(3).url);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(1).url);
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(0).url);

    // Active: "http://www.jolla.com" (3) -- original index in brachets
    // "https://sailfishos.org/sailfish-silica/index.html" (2)
    // "file:///opt/tests/sailfish-browser/manual/testpage.html" (1)
    // "http://sailfishos.org" (0)
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));
    QSignalSpy currentUrlChangedSpy(webContainer->currentTab(), SIGNAL(urlChanged()));
    for (int i = 0; i < originalTabOrder.count() - 1; ++i) {
        QString previousActiveUrl = webContainer->currentTab()->url();
        // Activate always last tab
        tabModel->activateTab(2);
        QCOMPARE(currentTabIdChangeSpy.count(), 1);
        QCOMPARE(currentUrlChangedSpy.count(), 1);
        QCOMPARE(webContainer->currentTab()->url(), originalTabOrder.at(i).url);
        // Previous active url should be pushed to the first model data index
        QModelIndex modelIndex = tabModel->createIndex(0, 0);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);
        currentTabIdChangeSpy.clear();
        currentUrlChangedSpy.clear();
    }

    QString previousActiveUrl = webContainer->currentTab()->url();
    // Activate by url
    tabModel->activateTab(originalTabOrder.at(3).url);
    QCOMPARE(currentTabIdChangeSpy.count(), 1);
    QCOMPARE(currentUrlChangedSpy.count(), 1);
    // Previous active url should be pushed to the first model data index
    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);

    // Full loop
    QCOMPARE(webContainer->currentTab()->url(), originalTabOrder.at(3).url);
    currentOrder = modelToStringList(tabModel);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(1).url);
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(0).url);
}



void tst_declarativetabmodel::remove()
{
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));
    QSignalSpy currentUrlChangedSpy(webContainer->currentTab(), SIGNAL(urlChanged()));
    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));
    QCOMPARE(tabModel->count(), originalTabOrder.count());
    QCOMPARE(webContainer->currentTab()->url(), originalTabOrder.at(3).url);
    tabModel->remove(1);

    QCOMPARE(currentTabIdChangeSpy.count(), 0);
    QCOMPARE(currentUrlChangedSpy.count(), 0);
    QCOMPARE(tabCountSpy.count(), 1);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabModel->rowCount(), 2);
    QCOMPARE(webContainer->currentTab()->url(), originalTabOrder.at(3).url);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(0).url);
}

void tst_declarativetabmodel::closeActiveTab()
{
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));
    QSignalSpy currentUrlChangedSpy(webContainer->currentTab(), SIGNAL(urlChanged()));
    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));

    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();

    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(webContainer->currentTab()->url(), originalTabOrder.at(3).url);
    tabModel->closeActiveTab();

    QCOMPARE(currentTabIdChangeSpy.count(), 1);
    QCOMPARE(currentUrlChangedSpy.count(), 1);
    QCOMPARE(tabCountSpy.count(), 1);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(tabModel->rowCount(), 1);
    QCOMPARE(webContainer->currentTab()->url(), originalTabOrder.at(2).url);
    QCOMPARE(webContainer->currentTab()->url(), newActiveUrl);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(0).url);
}

void tst_declarativetabmodel::forwardBackwardNavigation()
{
    tabModel->addTab("http://www.foobar.com/page1", "First Page");
    DeclarativeTab *tab = webContainer->currentTab();
    int tabId = tab->tabId();
    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));
    QSignalSpy forwardSpy(webContainer, SIGNAL(canGoForwardChanged()));
    QSignalSpy backSpy(webContainer, SIGNAL(canGoBackChanged()));

    QString url = "http://www.foobar.com/page2";
    tabModel->updateUrl(tabId, url);

    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(forwardSpy.count(), 0);

    QCOMPARE(tab->url(), url);
    QVERIFY(tab->title().isEmpty());
    QVERIFY(tab->thumbnailPath().isEmpty());

    backSpy.wait();
    QCOMPARE(backSpy.count(), 1);
    QVERIFY(webContainer->canGoBack());

    QVERIFY(tab->valid());
    QString title = "Second Page";
    tabModel->updateTitle(tabId, title);

    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 2);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(backSpy.count(), 1);
    QCOMPARE(tab->title(), title);

    webContainer->goBack();
    forwardSpy.wait();
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 3);

    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    url = "http://www.foobar.com/page1";
    title = "First Page";
    QCOMPARE(tab->url(), url);
    QCOMPARE(tab->title(), title);

    // Verify that spy counters will not update (1sec should be enough)
    QTest::qWait(1000);
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 3);

    webContainer->goForward();
    backSpy.wait();

    url = "http://www.foobar.com/page2";
    title = "Second Page";
    QCOMPARE(tab->url(), url);
    QCOMPARE(tab->title(), title);

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 3);
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 4);

    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    forwardSpy.clear();
    backSpy.clear();
    urlChangedSpy.clear();
    titleChangedSpy.clear();
    webContainer->goBack();
    forwardSpy.wait();

    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 1);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);

    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Mimic load that started from link clicking. False till back/forward invoked.
    tabModel->setBackForwardNavigation(false);
    url = "http://www.foobar.com/page3";
    tabModel->updateUrl(tabId, url);
    forwardSpy.wait();

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);

    QVERIFY(tab->title().isEmpty());
    QVERIFY(tab->thumbnailPath().isEmpty());
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    title = "Third page";
    tabModel->updateTitle(tabId, title);
    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 3);
    QCOMPARE(tab->title(), title);

    url = "http://www.foobar.com/page4";
    tabModel->updateUrl(tabId, url);
    QVERIFY(!tabModel->backForwardNavigation());
    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 4);
    QCOMPARE(tab->url(), url);
    QVERIFY(tab->title().isEmpty());
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    // page1, page3, page4
    // There items in tab history. Navigate twice back.
    webContainer->goBack();
    forwardSpy.wait();
    QCOMPARE(forwardSpy.count(), 3);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 4);
    QCOMPARE(titleChangedSpy.count(), 5);
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page3"));
    QCOMPARE(tab->title(), QString("Third page"));
    QVERIFY(webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Back to first page.
    webContainer->goBack();
    backSpy.wait();
    QCOMPARE(forwardSpy.count(), 3);
    QCOMPARE(backSpy.count(), 3);
    QCOMPARE(urlChangedSpy.count(), 5);
    QCOMPARE(titleChangedSpy.count(), 6);
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page1"));
    QCOMPARE(tab->title(), QString("First Page"));
    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Wait and check that all updates have come already
    QTest::qWait(1000);
    QCOMPARE(forwardSpy.count(), 3);
    QCOMPARE(backSpy.count(), 3);
    QCOMPARE(urlChangedSpy.count(), 5);
    QCOMPARE(titleChangedSpy.count(), 6);

    int expectedCount = tabModel->count() - 1;
    tabModel->removeTabById(tabId);
    QVERIFY(tabModel->count() == expectedCount);
    tabModel->setBackForwardNavigation(false);
}

void tst_declarativetabmodel::multipleTabsWithSameUrls()
{
    DeclarativeTab *tab = webContainer->currentTab();
    tab->invalidateTabData();
    QVERIFY(!tab->valid());

    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));

    // tab1: page1 ("First Page") and page2 ("")
    tabModel->addTab("http://www.foobar.com/page1", "First Page");
    int tab1 = tabModel->currentTabId();
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page1"));
    QCOMPARE(tab->title(), QString("First Page"));
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    tabModel->updateUrl(tab1, "http://www.foobar.com/page2");
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page2"));
    QVERIFY(tab->title().isEmpty());
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);

    // tab2: page1 ("First Page Too") and page2 ("Second Page Too")
    QSignalSpy tabUpdatedFromDb(DBManager::instance(), SIGNAL(tabChanged(Tab)));
    tabModel->addTab("http://www.foobar.com/page1", "First Page Too");
    int tab2 = tabModel->currentTabId();
    tabUpdatedFromDb.wait();
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page1"));
    QCOMPARE(tab->title(), QString("First Page Too"));
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 3);
    tabModel->updateUrl(tab2, "http://www.foobar.com/page2");
    tabUpdatedFromDb.wait();
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page2"));
    QVERIFY(tab->title().isEmpty());
    QCOMPARE(urlChangedSpy.count(), 4);
    QCOMPARE(titleChangedSpy.count(), 4);
    tabModel->updateTitle(tab2, QString("Second Page Too"));
    QCOMPARE(tab->title(), QString("Second Page Too"));
    QCOMPARE(urlChangedSpy.count(), 4);
    QCOMPARE(titleChangedSpy.count(), 5);

    // tab2: go back to page1 ("First Page Too")
    urlChangedSpy.clear();
    titleChangedSpy.clear();
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());
    QSignalSpy forwardSpy(webContainer, SIGNAL(canGoForwardChanged()));
    webContainer->goBack();
    forwardSpy.wait();
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page1"));
    QCOMPARE(tab->title(), QString("First Page Too"));
    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // tab1: go back to page1 ("First Page")
    urlChangedSpy.clear();
    titleChangedSpy.clear();
    forwardSpy.clear();
    tabModel->activateTabById(tab1);
    // Model has up-to-date data, no need to wait anything from database
    urlChangedSpy.wait();
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page2"));
    QVERIFY(tab->title().isEmpty());
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());
    webContainer->goBack();
    forwardSpy.wait();

    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);
    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(tab->url(), QString("http://www.foobar.com/page1"));
    QCOMPARE(tab->title(), QString("First Page"));
    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    int expectedCount = tabModel->count() - 2;
    tabModel->removeTabById(tab1);
    tabModel->removeTabById(tab2);
    QVERIFY(tabModel->count() == expectedCount);
    tabModel->setBackForwardNavigation(false);
}

void tst_declarativetabmodel::updateInvalidUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("tel") << "tel:+123456798";
    QTest::newRow("sms") << "sms:+123456798";
    QTest::newRow("mailto") << "mailto:joe@example.com";
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1";
    QTest::newRow("geo") << "geo:61.49464,23.77513";
    QTest::newRow("geo://") << "geo://61.49464,23.77513";
}

void tst_declarativetabmodel::updateInvalidUrls()
{
    QString expectedUrl = "http://foobar/invalid";
    DeclarativeTab *tab = webContainer->currentTab();
    tabModel->updateUrl(tab->tabId(), expectedUrl);

    QFETCH(QString, url);

    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    tabModel->updateUrl(tab->tabId(), url);

    QCOMPARE(urlChangedSpy.count(), 0);
    QCOMPARE(tab->url(), expectedUrl);
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

    DeclarativeTab *tab = webContainer->currentTab();
    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));

    tabModel->updateUrl(tab->tabId(), url);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(tab->url(), url);
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
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));

    QFETCH(QString, url);
    QFETCH(QString, title);
    int originalCount = tabModel->count();
    tabModel->addTab(url, title);

    QCOMPARE(tabModel->count(), originalCount);
    QCOMPARE(countChangeSpy.count(), 0);
    QCOMPARE(currentTabIdChangeSpy.count(), 0);
}

void tst_declarativetabmodel::updateTitle()
{

    DeclarativeTab *currentTab = webContainer->currentTab();
    tabModel->updateTitle(currentTab->tabId(), "A title something");

    QSignalSpy currentTabUrlSpy(currentTab, SIGNAL(urlChanged()));
    QSignalSpy currentTabTitleSpy(currentTab, SIGNAL(titleChanged()));
    QSignalSpy webContainerUrlSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy webContainerTitleSpy(webContainer, SIGNAL(titleChanged()));

    QString url = "http://foobar";
    tabModel->addTab(url, "");
    int tab1 = tabModel->currentTabId();

    QCOMPARE(currentTab->url(), url);
    QVERIFY(currentTab->title().isEmpty());
    QCOMPARE(webContainer->url(), url);
    QVERIFY(webContainer->title().isEmpty());
    QCOMPARE(currentTabUrlSpy.count(), 1);
    QCOMPARE(currentTabTitleSpy.count(), 1);
    QCOMPARE(webContainerUrlSpy.count(), 1);
    QCOMPARE(webContainerTitleSpy.count(), 1);

    QString title = "FooBar Title";
    tabModel->updateTitle(tab1, title);
    QCOMPARE(currentTab->title(), title);
    QCOMPARE(webContainer->title(), title);
    QCOMPARE(currentTabTitleSpy.count(), 2);
    QCOMPARE(webContainerTitleSpy.count(), 2);

    tabModel->updateTitle(tab1, "");
    QVERIFY(currentTab->title().isEmpty());
    QVERIFY(webContainer->title().isEmpty());
    QCOMPARE(currentTabTitleSpy.count(), 3);
    QCOMPARE(webContainerTitleSpy.count(), 3);

    // Add a new tab with same url and change title "" -> "FooBar"
    title = "FooBar";
    tabModel->addTab(url, title);
    int tab2 = tabModel->currentTabId();

    QCOMPARE(currentTab->url(), url);
    QCOMPARE(currentTab->title(), title);
    QCOMPARE(webContainer->url(), url);
    QCOMPARE(webContainer->title(), title);

    QCOMPARE(currentTabUrlSpy.count(), 1);
    QCOMPARE(currentTabTitleSpy.count(), 4);
    QCOMPARE(webContainerUrlSpy.count(), 1);
    QCOMPARE(webContainerTitleSpy.count(), 4);

    // Tab1 in model should not be changed after tab added.
    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), url);
    QVERIFY(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString().isEmpty());

    title = "FooBar Two";
    tabModel->updateTitle(tab2, title);
    QCOMPARE(currentTab->title(), title);
    QCOMPARE(webContainer->title(), title);
    QCOMPARE(currentTabTitleSpy.count(), 5);
    QCOMPARE(webContainerTitleSpy.count(), 5);

    // Tab1 in model should not be changed after title changed for tab2.
    modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), url);
    QVERIFY(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString().isEmpty());

    // No title change should not trigger change.
    tabModel->updateTitle(tab2, title);
    QCOMPARE(currentTabTitleSpy.count(), 5);
    QCOMPARE(webContainerTitleSpy.count(), 5);
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

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_declarativetabmodel testcase;
    qmlRegisterUncreatableType<DeclarativeTab>("Sailfish.Browser", 1, 0, "Tab", "");
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    qmlRegisterType<DeclarativeWebContainer>("Sailfish.Browser", 1, 0, "WebContainer");
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativetabmodel.moc"
