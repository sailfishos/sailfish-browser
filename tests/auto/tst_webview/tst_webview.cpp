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
#include <QQuickView>
#include <qmozcontext.h>

#include "declarativehistorymodel.h"
#include "declarativetabmodel.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "declarativewebviewcreator.h"
#include "declarativewebutils.h"
#include "testobject.h"

class tst_webview : public TestObject
{
    Q_OBJECT

public:
    tst_webview();

private slots:
    void initTestCase();
    void testNewTab_data();
    void testNewTab();
    void testActivateTab();
    void testCloseActiveTab();
    void testRemoveTab();
    void testUrlLoading();
    void testLiveTabCount_data();
    void testLiveTabCount();
    void forwardBackwardNavigation();
    void clear();
    void restart();
    void cleanupTestCase();

private:
    QString formatUrl(QString fileName) const;
    void verifyHistory();

    DeclarativeHistoryModel *historyModel;
    DeclarativeTabModel *tabModel;
    DeclarativeWebContainer *webContainer;
    QString baseUrl;
};


tst_webview::tst_webview()
    : TestObject()
    , historyModel(0)
    , tabModel(0)
    , webContainer(0)
{
}

void tst_webview::initTestCase()
{
    init(QUrl("qrc:///tst_webview.qml"));
    webContainer = TestObject::qmlObject<DeclarativeWebContainer>("webView");
    QVERIFY(webContainer);
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));

    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    QVERIFY(tabModel);
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));

    historyModel = TestObject::qmlObject<DeclarativeHistoryModel>("historyModel");
    QVERIFY(historyModel);

    waitSignals(loadingChanged, 2);

    DeclarativeWebPage *webPage = webContainer->webPage();
    QVERIFY(webPage);
    QCOMPARE(webPage->url().toString(), DeclarativeWebUtils::instance()->homePage());
    QCOMPARE(webPage->title(), QString("TestPage"));
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(webContainer->m_webPages->count(), 1);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 1);

    baseUrl = QUrl(DeclarativeWebUtils::instance()->homePage()).toLocalFile();
    baseUrl = QFileInfo(baseUrl).canonicalPath();

    waitSignals(tabAddedSpy, 1);
    QCOMPARE(tabAddedSpy.count(), 1);
}

void tst_webview::testNewTab_data()
{
    QTest::addColumn<QString>("newUrl");
    QTest::addColumn<QString>("newTitle");
    QTest::addColumn<QString>("expectedTitle");
    QTest::addColumn<int>("expectedTitleChangeCount");
    QTest::addColumn<QStringList>("activeTabs");

    QString homePage = DeclarativeWebUtils::instance()->homePage();
    QStringList activeTabOrder = QStringList() << formatUrl("testselect.html")
                                               << homePage;
    QTest::newRow("testselect") << formatUrl("testselect.html") << "TestSelect"
                                << "TestSelect" << 2 << activeTabOrder;

    activeTabOrder = QStringList() << formatUrl("testuseragent.html")
                                   << formatUrl("testselect.html")
                                   << homePage;
    QTest::newRow("testuseragent") << formatUrl("testuseragent.html") << "TestUserAgent"
                                   << "TestUserAgent" << 2 << activeTabOrder;

    // The new tab added without newTitle -> title loaded from testnavigation.html page.
    // Same as creating a new tab by typing the address to the url field.
    activeTabOrder = QStringList() << formatUrl("testnavigation.html")
                                   << formatUrl("testuseragent.html")
                                   << formatUrl("testselect.html")
                                   << homePage;
    QTest::newRow("testnavigation") << formatUrl("testnavigation.html") << ""
                                    << "TestNavigation" << 2 << activeTabOrder;
}

void tst_webview::testNewTab()
{
    QFETCH(QString, newUrl);
    QFETCH(QString, newTitle);
    QFETCH(QString, expectedTitle);
    QFETCH(int, expectedTitleChangeCount);

    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));

    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));

    int expectedTabCount = tabModel->count() + 1;

    DeclarativeWebPage *previousPage = webContainer->webPage();
    QVERIFY(previousPage);

    // Mimic favorite opening to a new tab. Favorites can have both url and title and when entering
    // url through virtual keyboard only url is provided.
    tabModel->newTab(newUrl, newTitle);

    // Wait for MozView instance change.
    waitSignals(contentItemSpy, 1);

    QVERIFY(webContainer->webPage());
    QVERIFY(previousPage != webContainer->webPage());
    QCOMPARE(contentItemSpy.count(), 1);
    waitSignals(loadingChanged, 2);

    // These are difficult to spy at this point as url changes almost immediately
    // and contentItem is changed in newTab code path.
    QCOMPARE(webContainer->webPage()->url().toString(), newUrl);
    QCOMPARE(webContainer->webPage()->title(), expectedTitle);

    // ~last in the sequence of adding a new tab.
    waitSignals(tabAddedSpy, 1);

    // Url and title signals are emitted always twice. Empty url/title and loaded url/title.
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), expectedTitleChangeCount);
    QCOMPARE(tabCountSpy.count(), 1);
    QCOMPARE(tabModel->count(), expectedTabCount);
    QCOMPARE(webContainer->url(), newUrl);
    QCOMPARE(webContainer->title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), 1);

    // Signaled always when tab is changed.
    QList<QVariant> arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(1).toInt();
    QCOMPARE(activatedTabId, webContainer->webPage()->tabId());

    // Signaled only when tab added.
    QCOMPARE(tabAddedSpy.count(), 1);
    arguments = tabAddedSpy.takeFirst();
    int addedTabId = arguments.at(0).toInt();
    QCOMPARE(addedTabId, webContainer->webPage()->tabId());

    QFETCH(QStringList, activeTabs);
    QCOMPARE(webContainer->m_webPages->count(), activeTabs.count());
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), activeTabs.count());
    QCOMPARE(webContainer->webPage()->url().toString(), activeTabs.at(0));
    activeTabs.removeFirst();
    for (int i = 0; i < activeTabs.count(); ++i) {
        QModelIndex modelIndex = tabModel->createIndex(i, 0);
        QString url = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
        QCOMPARE(url, activeTabs.at(i));
    }
}

/*!
    Test tab activation, url and title change, and loading status. Tab order
    changing is tested by tst_declarativetabmodel.
*/
void tst_webview::testActivateTab()
{
    // Active tabs in order:
    // "testnavigation.html", "TestNavigation" (active)
    // "testuseragent.html", "TestUserAgent" (0)
    // "testselect.html", "TestSelect" (1)
    // "testpage.html", "TestPage" (2)
    QCOMPARE(tabModel->count(), 4);
    QCOMPARE(webContainer->m_webPages->count(), 4);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 4);

    // "testpage.html", "TestPage"
    QModelIndex modelIndex = tabModel->createIndex(2, 0);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
    QString newActiveTitle = tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString();
    int newActiveTabId = tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt();

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    tabModel->activateTab(2);
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(contentItemSpy.count(), 1);
    // Tab already loaded.
    QVERIFY(!webContainer->webPage()->loading());
    QCOMPARE(webContainer->webPage()->loadProgress(), 100);

    QCOMPARE(webContainer->webPage()->tabId(), newActiveTabId);
    QCOMPARE(webContainer->url(), newActiveUrl);
    QCOMPARE(webContainer->title(), newActiveTitle);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(webContainer->webPage()->url().toString(), newActiveUrl);
    QCOMPARE(webContainer->webPage()->title(), newActiveTitle);

    // Signaled always when tab is changed.
    QList<QVariant> arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(1).toInt();
    QCOMPARE(activatedTabId, webContainer->webPage()->tabId());
}

void tst_webview::testCloseActiveTab()
{
    // Active tabs in order:
    // "testpage.html", "TestPage" (active)
    // "testnavigation.html", "TestNavigation" (0)
    // "testuseragent.html", "TestUserAgent" (1)
    // "testselect.html", "TestSelect" (2)

    // "testnavigation.html", "TestNavigation"
    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
    QString newActiveTitle = tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString();
    int newActiveTabId = tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt();

    int previousTabId = webContainer->webPage()->tabId();

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    tabModel->closeActiveTab();
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(tabClosedSpy.count(), 1);
    QList<QVariant> arguments = tabClosedSpy.takeFirst();
    int closedTabId = arguments.at(0).toInt();
    QCOMPARE(closedTabId, previousTabId);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(webContainer->m_webPages->count(), 3);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 3);
    // Two signals: closeActiveTab causes contentItem to be destroyed and second signal comes
    // when the first item from model is made as active tab.
    QCOMPARE(contentItemSpy.count(), 2);
    // Tab already loaded.
    QVERIFY(!webContainer->webPage()->loading());
    QCOMPARE(webContainer->webPage()->loadProgress(), 100);

    QCOMPARE(webContainer->webPage()->tabId(), newActiveTabId);
    QCOMPARE(webContainer->url(), newActiveUrl);
    QCOMPARE(webContainer->title(), newActiveTitle);
    // Two signals: closeActiveTab causes contentItem to be destroyed. Thus, both url and title
    // are update signaled. Second url/title changed signal comes
    // when the first item from model is made as active tab.
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);
    QCOMPARE(webContainer->webPage()->url().toString(), newActiveUrl);
    QCOMPARE(webContainer->webPage()->title(), newActiveTitle);

    // Signaled always when tab is changed.
    arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(1).toInt();
    QCOMPARE(activatedTabId, webContainer->webPage()->tabId());
}

void tst_webview::testRemoveTab()
{
    // Active tabs in order:
    // "testnavigation.html", "TestNavigation" (active)
    // "testuseragent.html", "TestUserAgent" (0)
    // "testselect.html", "TestSelect" (1)

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));

    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    int tabIdOfIndexZero = tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt();
    tabModel->remove(0);

    QCOMPARE(tabClosedSpy.count(), 1);
    QList<QVariant> arguments = tabClosedSpy.takeFirst();
    int closedTabId = arguments.at(0).toInt();
    QCOMPARE(closedTabId, tabIdOfIndexZero);
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(webContainer->m_webPages->count(), 2);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 2);

    QVERIFY(activeTabChangedSpy.count() == 0);
    QVERIFY(urlChangedSpy.count() == 0);
    QVERIFY(titleChangedSpy.count() == 0);
    QVERIFY(contentItemSpy.count() == 0);
}

void tst_webview::testUrlLoading()
{
    // Active tabs in order:
    // "testnavigation.html", "TestNavigation" (active)
    // "testselect.html", "TestSelect" (0)

    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy pageUrlChangedSpy(webContainer->webPage(), SIGNAL(urlChanged()));
    QSignalSpy pageTitleChangedSpy(webContainer->webPage(), SIGNAL(titleChanged()));
    QSignalSpy backNavigationChangedSpy(webContainer, SIGNAL(canGoBackChanged()));


    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));

    QString newUrl = formatUrl("testurlscheme.html");
    QString newTitle = "TestUrlScheme";

    // Mimic favorite opening to already opened tab.
    emit webContainer->triggerLoad(newUrl, newTitle);
    waitSignals(loadingChanged, 2);

    QCOMPARE(contentItemSpy.count(), 0);

    QCOMPARE(pageUrlChangedSpy.count(), 1);
    QCOMPARE(pageTitleChangedSpy.count(), 1);
    QCOMPARE(webContainer->webPage()->url().toString(), newUrl);
    QCOMPARE(webContainer->webPage()->title(), newTitle);

    // When loading a page, url and title changes only based on HTML content.
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(webContainer->url(), newUrl);
    QCOMPARE(webContainer->title(), newTitle);
    QCOMPARE(webContainer->webPage()->url().toString(), newUrl);
    QCOMPARE(webContainer->webPage()->title(), newTitle);

    waitSignals(backNavigationChangedSpy, 1);
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(webContainer->m_webPages->count(), 2);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 2);
}

void tst_webview::testLiveTabCount_data()
{
    QTest::addColumn<QString>("newUrl");
    QTest::addColumn<int>("expectedTabCount");
    QTest::addColumn<int>("liveTabCount");
    QTest::newRow("testuseragent") << formatUrl("testuseragent.html") << 3 << 3;
    QTest::newRow("testinputfocus") << formatUrl("testinputfocus.html") << 4 << 4;
    QTest::newRow("testurlscheme") << formatUrl("testurlscheme.html") << 5 << 5;
    QTest::newRow("testwindowopen") << formatUrl("testwindowopen.html") << 6 << 5;
    QTest::newRow("testwebprompts") << formatUrl("testwebprompts.html") << 7 << 5;
}

void tst_webview::testLiveTabCount()
{
    QFETCH(QString, newUrl);
    QFETCH(int, expectedTabCount);
    QFETCH(int, liveTabCount);

    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));

    tabModel->newTab(newUrl, "");
    waitSignals(loadingChanged, 2);

    // ~last in the sequence of adding a new tab.
    waitSignals(tabAddedSpy, 1);

    // Url and title signals emitted are only once.
    QCOMPARE(tabCountSpy.count(), 1);
    QCOMPARE(tabModel->count(), expectedTabCount);
    QCOMPARE(activeTabChangedSpy.count(), 1);

    QCOMPARE(webContainer->m_webPages->count(), liveTabCount);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), expectedTabCount);
}

void tst_webview::forwardBackwardNavigation()
{
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy forwardSpy(webContainer, SIGNAL(canGoForwardChanged()));
    QSignalSpy backSpy(webContainer, SIGNAL(canGoBackChanged()));
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,int)));

    QString url = formatUrl("testwindowopen.html");
    QString title = "Test window opening";
    webContainer->webPage()->loadTab(url, false);
    waitSignals(loadingChanged, 2);
    waitSignals(activeTabChangedSpy, 1);

    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(webContainer->url(), url);
    QCOMPARE(webContainer->title(), title);

    QCOMPARE(backSpy.count(), 1);
    QVERIFY(webContainer->canGoBack());
    webContainer->goBack();
    QTest::qWait(1000);
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);

    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Verify that spy counters will not update (1sec should be enough)
    QTest::qWait(1000);
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);

    webContainer->goForward();
    QTest::qWait(1000);

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 3);
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 3);

    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    forwardSpy.clear();
    backSpy.clear();
    urlChangedSpy.clear();
    titleChangedSpy.clear();
    webContainer->goBack();
    QTest::qWait(1000);

    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 1);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);

    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());
    url = formatUrl("testurlscheme.html");
    title = "TestUrlScheme";
    webContainer->webPage()->loadTab(url, false);
    QTest::qWait(1000);

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);

    QCOMPARE(webContainer->url(), url);
    QCOMPARE(webContainer->title(), title);
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    url = formatUrl("testuseragent.html");
    title = "TestUserAgent";
    webContainer->webPage()->loadTab(url, false);
    QTest::qWait(1000);

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 3);
    QCOMPARE(webContainer->url(), url);
    QCOMPARE(webContainer->title(), title);
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    // testwebprompts.html, testurlscheme.html, testuseragent.html
    // Navigate twice back.
    webContainer->goBack();
    QTest::qWait(1000);
    QCOMPARE(forwardSpy.count(), 3);
    // When goBack / goForward is called respected navigation direction is blocked immediately.
    // Only after information is returned from database we might unlock navigation direction.
    QCOMPARE(backSpy.count(), 4);
    QCOMPARE(urlChangedSpy.count(), 4);
    QCOMPARE(titleChangedSpy.count(), 4);
    QCOMPARE(webContainer->url(), formatUrl("testurlscheme.html"));
    QCOMPARE(webContainer->title(), QString("TestUrlScheme"));
    QVERIFY(webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Back to first page.
    webContainer->goBack();
    QTest::qWait(1000);
    QCOMPARE(forwardSpy.count(), 3);
    QCOMPARE(backSpy.count(), 5);
    QCOMPARE(urlChangedSpy.count(), 5);
    QCOMPARE(titleChangedSpy.count(), 5);
    QCOMPARE(webContainer->url(), formatUrl("testwebprompts.html"));
    QCOMPARE(webContainer->title(), QString("Test Web Prompts"));
    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Same as in previous case
    QCOMPARE(tabModel->count(), 7);
    QCOMPARE(webContainer->m_webPages->count(), webContainer->m_maxLiveTabCount);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 7);
}

void tst_webview::clear()
{
    QSignalSpy tabsCleared(tabModel, SIGNAL(tabsCleared()));
    historyModel->clear();
    tabModel->clear();
    QTest::qWait(1000);
    waitSignals(tabsCleared, 1);
    QVERIFY(historyModel->rowCount() == 0);
    QVERIFY(webContainer->m_webPages->count() == 0);
    QVERIFY(webContainer->m_webPages->m_activePages.count() == 0);
    QVERIFY(!webContainer->m_webPages->m_activePage);
    QVERIFY(!webContainer->m_webPage);
}

void tst_webview::restart()
{
    // Title "TestPage"
    QString testPageUrl = formatUrl("testpage.html");
    tabModel->newTab(testPageUrl, "");
    QTest::qWait(1000);

    QCOMPARE(tabModel->activeTab().url(), testPageUrl);
    QCOMPARE(webContainer->url(), testPageUrl);

    // Title "TestUserAgent"
    QString testUserAgentUrl = formatUrl("testuseragent.html");
    webContainer->webPage()->loadTab(testUserAgentUrl, false);
    QTest::qWait(1000);

    QCOMPARE(tabModel->activeTab().url(), testUserAgentUrl);
    QCOMPARE(webContainer->url(), testUserAgentUrl);
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(webContainer->m_webPages->count(), 1);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 1);

    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    // Before restart
    verifyHistory();

    delete tabModel;
    tabModel = 0;
    delete historyModel;
    historyModel = 0;
    delete webContainer;
    webContainer = 0;

    setTestData(EMPTY_QML);
    setTestUrl(QUrl("qrc:///tst_webview.qml"));
    QTest::qWait(1000);

    webContainer = TestObject::qmlObject<DeclarativeWebContainer>("webView");
    historyModel = TestObject::qmlObject<DeclarativeHistoryModel>("historyModel");
    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    QVERIFY(tabModel->count() == 1);
    QCOMPARE(tabModel->activeTab().url(), testUserAgentUrl);
    QCOMPARE(webContainer->url(), testUserAgentUrl);

    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    // After restart
    verifyHistory();

    webContainer->goBack();
    QTest::qWait(1000);

    // After back navigation
    verifyHistory();

    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    QCOMPARE(tabModel->activeTab().url(), testPageUrl);
    QCOMPARE(webContainer->url(), testPageUrl);
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(webContainer->m_webPages->count(), 1);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 1);
}

void tst_webview::cleanupTestCase()
{
    QTest::qWait(1000);

    tabModel->clear();
    QVERIFY(tabModel->count() == 0);
    QVERIFY(webContainer->url().isEmpty());
    QVERIFY(webContainer->title().isEmpty());

    // Wait for event loop of db manager
    QTest::qWait(1000);
    QString dbFileName = QString("%1/%2")
            .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(dbFileName);
    QVERIFY(dbFile.remove());
    QMozContext::GetInstance()->stopEmbedding();
}

/*!
    Format url from \a fileName that is given as relative to the homePage.
*/
QString tst_webview::formatUrl(QString fileName) const
{
    return QUrl::fromLocalFile(baseUrl + "/" + fileName).toString();
}

void tst_webview::verifyHistory()
{
    QString testPageUrl = formatUrl("testpage.html");
    QString testUserAgentUrl = formatUrl("testuseragent.html");
    historyModel->search("");
    QTest::qWait(1000);
    QCOMPARE(historyModel->rowCount(), 2);
    QModelIndex modelIndex = historyModel->createIndex(0, 0);
    QCOMPARE(historyModel->data(modelIndex, DeclarativeHistoryModel::TitleRole).toString(),
             QString("TestPage"));
    QCOMPARE(historyModel->data(modelIndex, DeclarativeHistoryModel::UrlRole).toString(),
             testPageUrl);

    modelIndex = historyModel->createIndex(1, 0);
    QCOMPARE(historyModel->data(modelIndex, DeclarativeHistoryModel::TitleRole).toString(),
             QString("TestUserAgent"));
    QCOMPARE(historyModel->data(modelIndex, DeclarativeHistoryModel::UrlRole).toString(),
             testUserAgentUrl);

}

int main(int argc, char *argv[])
{
    setenv("USE_ASYNC", "1", 1);
    setenv("QML_BAD_GUI_RENDER_LOOP", "1", 1);

    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_webview testcase;
    testcase.setContextProperty("WebUtils", DeclarativeWebUtils::instance());
    testcase.setContextProperty("MozContext", QMozContext::GetInstance());

    qmlRegisterType<DeclarativeHistoryModel>("Sailfish.Browser", 1, 0, "HistoryModel");
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    qmlRegisterType<DeclarativeWebContainer>("Sailfish.Browser", 1, 0, "WebContainer");
    qmlRegisterType<DeclarativeWebPage>("Sailfish.Browser", 1, 0, "WebPage");
    qmlRegisterType<DeclarativeWebViewCreator>("Sailfish.Browser", 1, 0, "WebViewCreator");

    QString componentPath(DEFAULT_COMPONENTS_PATH);
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteBinComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteJSComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteJSScripts.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteOverrides.manifest"));

    QTimer::singleShot(0, QMozContext::GetInstance(), SLOT(runEmbedding()));

    return QTest::qExec(&testcase, argc, argv);
}

#include "tst_webview.moc"
