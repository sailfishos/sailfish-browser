/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014 - 2021 Jolla Ltd.
 * Copyright (c) 2021 Open Mobile Platform LLC.
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>
#include <QQuickView>
#include <QQmlEngine>
#include <webengine.h>
#include <webenginesettings.h>

// Registered QML types
#include "dbmanager.h"
#include "declarativehistorymodel.h"
#include "persistenttabmodel.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "declarativewebpagecreator.h"
#include "privatetabmodel.h"
#include "declarativebookmarkmodel.h"
#include "bookmarkfiltermodel.h"
#include "declarativeloginmodel.h"
#include "loginfiltermodel.h"
#include "faviconmanager.h"
#include "downloadstatus.h"
#include "desktopbookmarkwriter.h"
#include "datafetcher.h"
#include "inputregion.h"
#include "searchenginemodel.h"
#include "settingmanager.h"

#include "declarativewebutils.h"
#include "webpages.h"
#include "browserpaths.h"
#include "testobject.h"

#ifndef START_URL
// This should be defined in the build scripts
#define START_URL "file:///opt/tests/sailfish-browser/manual/testpage.html"
#endif

static const QString startPage(START_URL);

static QObject *search_model_factory(QQmlEngine *, QJSEngine *)
{
   return new SearchEngineModel;
}

static QObject *faviconmanager_factory(QQmlEngine *, QJSEngine *)
{
   return FaviconManager::instance();
}

class tst_webview : public TestObject
{
    Q_OBJECT

public:
    tst_webview();

    DeclarativeHistoryModel *historyModel;
    DeclarativeTabModel *tabModel;
    DeclarativeWebContainer *webContainer;

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
    void changeTabAndLoad();

private:
    void load(QString url, bool expectTitleChange, int waitUrlSignals = 2);
    void goBack();
    void goForward();

    QString formatUrl(QString fileName) const;
    bool verifyHistory(QList<TestTab> &historyOrder);

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
    QQmlEngine::setObjectOwnership(webContainer, QQmlEngine::CppOwnership);
    QVERIFY(webContainer);
    QSignalSpy completedChanged(webContainer, SIGNAL(completedChanged()));
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));
    QSignalSpy urlChanged(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChanged(webContainer, SIGNAL(titleChanged()));
    QSignalSpy testSetupReadyChanged(rootObject(), SIGNAL(testSetupReadyChanged()));

    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    tabModel->m_unittestMode = true;
    QVERIFY(tabModel);
    tabModel->clear();
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));

    historyModel = TestObject::qmlObject<DeclarativeHistoryModel>("historyModel");
    QVERIFY(historyModel);

    QTest::qWait(500);

    qDebug() << "Initial page: " << startPage;
    webContainer->load(startPage);

    waitSignals(completedChanged, 1);
    waitSignals(loadingChanged, 2);
    waitSignals(tabAddedSpy, 1);
    waitSignals(urlChanged, 1);
    waitSignals(titleChanged, 1);
    waitSignals(testSetupReadyChanged, 1);

    QVERIFY(rootObject()->property("testSetupReady").toBool());
    QTest::qWait(500);

    DeclarativeWebPage *webPage = webContainer->webPage();
    QVERIFY(webPage);
    QCOMPARE(webPage->url().toString(), startPage);
    QCOMPARE(webPage->title(), QString("TestPage"));
    QCOMPARE(webContainer->url(), startPage);
    QCOMPARE(webContainer->title(), QString("TestPage"));
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(webContainer->m_webPages->count(), 1);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 1);

    baseUrl = QUrl(startPage).toLocalFile();
    baseUrl = QFileInfo(baseUrl).canonicalPath();

    QCOMPARE(tabAddedSpy.count(), 1);

    SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();
    webEngineSettings->setPreference(QString("media.resource_handler_disabled"), QVariant(true));
}

void tst_webview::testNewTab_data()
{
    QTest::addColumn<QString>("newUrl");
    QTest::addColumn<QString>("newTitle");
    QTest::addColumn<QString>("expectedTitle");
    QTest::addColumn<int>("expectedTitleChangeCount");
    QTest::addColumn<int>("activeTabIndex");
    QTest::addColumn<QStringList>("activeTabs");

    QString homePage = startPage;
    QStringList activeTabOrder = QStringList() << homePage
                                               << formatUrl("testselect.html");
    QTest::newRow("testselect") << formatUrl("testselect.html") << "TestSelect"
                                << "TestSelect" << 2 << 1 << activeTabOrder;

    activeTabOrder << formatUrl("testuseragent.html");
    QTest::newRow("testuseragent") << formatUrl("testuseragent.html") << "TestUserAgent"
                                   << "TestUserAgent" << 2 << 2 << activeTabOrder;

    // The new tab added without newTitle -> title loaded from testnavigation.html page.
    // Same as creating a new tab by typing the address to the url field.
    activeTabOrder << formatUrl("testnavigation.html");
    QTest::newRow("testnavigation") << formatUrl("testnavigation.html") << ""
                                    << "TestNavigation" << 2 << 3 << activeTabOrder;
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
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));

    int expectedTabCount = tabModel->count() + 1;

    DeclarativeWebPage *previousPage = webContainer->webPage();
    QVERIFY(previousPage);

    // Mimic favorite opening to a new tab. Favorites can have both url and title and when entering
    // url through virtual keyboard only url is provided.
    tabModel->newTab(newUrl);

    // Wait for MozView instance change.
    waitSignals(contentItemSpy, 1);

    QVERIFY(webContainer->webPage());
    QVERIFY(previousPage != webContainer->webPage());
    QCOMPARE(contentItemSpy.count(), 1);
    QSignalSpy pageUrlChangedSpy(webContainer->webPage(), SIGNAL(urlChanged()));
    QSignalSpy pageTitleChangedSpy(webContainer->webPage(), SIGNAL(titleChanged()));
    QSignalSpy pageLoadedChangedSpy(webContainer->webPage(), SIGNAL(loadedChanged()));

    waitSignals(loadingChanged, 2);
    waitSignals(pageUrlChangedSpy, 1);
    waitSignals(pageTitleChangedSpy, 1);
    waitSignals(pageLoadedChangedSpy, 1);

    // These are difficult to spy at this point as url changes almost immediately
    // and contentItem is changed in newTab code path.
    QCOMPARE(webContainer->webPage()->url().toString(), newUrl);
    QCOMPARE(webContainer->webPage()->title(), expectedTitle);

    // ~last in the sequence of adding a new tab.
    waitSignals(tabAddedSpy, 1);

    // Url and title signals are emitted 3 times. When activating, requested url, and resolved url.
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), expectedTitleChangeCount);
    QCOMPARE(tabCountSpy.count(), 1);
    QCOMPARE(tabModel->count(), expectedTabCount);
    QCOMPARE(webContainer->url(), newUrl);
    QCOMPARE(webContainer->title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), 1);

    // Signaled always when tab is changed.
    QList<QVariant> arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(0).toInt();
    QCOMPARE(activatedTabId, webContainer->webPage()->tabId());

    // Signaled only when tab added.
    QCOMPARE(tabAddedSpy.count(), 1);
    arguments = tabAddedSpy.takeFirst();
    int addedTabId = arguments.at(0).toInt();
    QCOMPARE(addedTabId, webContainer->webPage()->tabId());

    QFETCH(QStringList, activeTabs);
    QFETCH(int, activeTabIndex);
    QCOMPARE(webContainer->m_webPages->count(), activeTabs.count());
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), activeTabs.count());
    QCOMPARE(webContainer->webPage()->url().toString(), activeTabs.at(activeTabIndex));
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
    // "testpage.html", "TestPage" (0), used(4)
    // "testselect.html", "TestSelect" (1), used(3)
    // "testuseragent.html", "TestUserAgent" (2), used(2)
    // "testnavigation.html", "TestNavigation" (3 - active), used (1)
    QCOMPARE(tabModel->count(), 4);
    QCOMPARE(webContainer->m_webPages->count(), 4);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 4);

    // "testpage.html", "TestPage"
    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
    QString newActiveTitle = tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString();
    int newActiveTabId = tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt();

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    tabModel->activateTab(0);
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(contentItemSpy.count(), 1);
    // Tab already loaded.
    QVERIFY(!webContainer->webPage()->loading());
    QCOMPARE(webContainer->webPage()->loadProgress(), 100);

    QCOMPARE(webContainer->webPage()->tabId(), newActiveTabId);
    QCOMPARE(webContainer->url(), newActiveUrl);
    QVERIFY(webContainer->url().endsWith("testpage.html"));
    QCOMPARE(webContainer->title(), newActiveTitle);
    QCOMPARE(webContainer->title(), QString("TestPage"));
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(webContainer->webPage()->url().toString(), newActiveUrl);
    QVERIFY(webContainer->webPage()->url().toString().endsWith("testpage.html"));
    QCOMPARE(webContainer->webPage()->title(), newActiveTitle);

    // Signaled always when tab is changed.
    QList<QVariant> arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(0).toInt();
    QCOMPARE(activatedTabId, webContainer->webPage()->tabId());
}

void tst_webview::testCloseActiveTab()
{
    // Active tabs in order:
    // "testpage.html", "TestPage" (0 - active), used (1)
    // "testselect.html", "TestSelect" (1), used (4)
    // "testuseragent.html", "TestUserAgent" (2), used (3)
    // "testnavigation.html", "TestNavigation" (3), used (2)

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    // Close will nullify contentItem and prepares new active tab but doesn't load/change
    // contentItem item.
    int previousTabId = webContainer->webPage()->tabId();
    tabModel->closeActiveTab();
    // Updates contentItem to match index zero.
    Tab tab;
    tab.setTabId(webContainer->tabId());
    webContainer->activatePage(tab);
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(tabClosedSpy.count(), 1);
    QList<QVariant> arguments = tabClosedSpy.takeFirst();
    int closedTabId = arguments.at(0).toInt();
    QCOMPARE(closedTabId, previousTabId);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(webContainer->m_webPages->count(), 3);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 3);
    QCOMPARE(contentItemSpy.count(), 1);
    // Tab already loaded.
    QVERIFY(!webContainer->webPage()->loading());
    QCOMPARE(webContainer->webPage()->loadProgress(), 100);

    // "testnavigation.html", "TestNavigation" is the new active tab as it was used before the testpage.html.
    QModelIndex modelIndex = tabModel->createIndex(tabModel->count() - 1, tabModel->count() - 1);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
    QString newActiveTitle = tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString();
    int newActiveTabId = tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt();

    QCOMPARE(webContainer->webPage()->tabId(), newActiveTabId);
    QCOMPARE(webContainer->url(), newActiveUrl);
    QVERIFY(webContainer->url().endsWith("testnavigation.html"));
    QCOMPARE(webContainer->title(), newActiveTitle);
    QCOMPARE(webContainer->title(), QString("TestNavigation"));
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(webContainer->webPage()->url().toString(), newActiveUrl);
    QVERIFY(webContainer->webPage()->url().toString().endsWith("testnavigation.html"));
    QCOMPARE(webContainer->webPage()->title(), newActiveTitle);
    QCOMPARE(webContainer->webPage()->title(), QString("TestNavigation"));

    // Signaled always when tab is changed.
    arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(0).toInt();
    QCOMPARE(activatedTabId, webContainer->webPage()->tabId());
}

void tst_webview::testRemoveTab()
{
    // Active tabs in order:
    // "testselect.html", "TestSelect" (0 - active)
    // "testuseragent.html", "TestUserAgent" (1)
    // "testnavigation.html", "TestNavigation" (2)

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));

    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    int removeIndex = 1;
    QModelIndex modelIndex = tabModel->createIndex(removeIndex, 0);
    int tabIdOfIndexZero = tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt();
    tabModel->remove(removeIndex);

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
    // "testselect.html", "TestSelect" (0 - active)
    // "testnavigation.html", "TestNavigation" (1)

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
    webContainer->load(newUrl);
    waitSignals(loadingChanged, 2);
    waitSignals(pageUrlChangedSpy, 1);
    waitSignals(pageTitleChangedSpy, 1);

    QCOMPARE(contentItemSpy.count(), 0);

    // Requested and resolved urls.
    QCOMPARE(pageUrlChangedSpy.count(), 2);
    QCOMPARE(pageTitleChangedSpy.count(), 1);
    QCOMPARE(webContainer->webPage()->url().toString(), newUrl);
    QCOMPARE(webContainer->webPage()->title(), newTitle);

    // When loading a page, url and title changes only based on HTML content.
    // Load itself triggers urlChanged based on the requested url.
    QCOMPARE(urlChangedSpy.count(), 2);
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
    QTest::addColumn<int>("willHaveTitle");
    QTest::addColumn<int>("hadTitle"); // Previous page had title
    QTest::newRow("testuseragent") << formatUrl("testuseragent.html") << 3 << 3 << 1 << 1;
    QTest::newRow("testinputfocus") << formatUrl("testinputfocus.html") << 4 << 4 << 0 << 1;
    QTest::newRow("testurlscheme") << formatUrl("testurlscheme.html") << 5 << 5 << 1 << 0;
    QTest::newRow("testwindowopen") << formatUrl("testwindowopen.html") << 6 << 5 << 1 << 1;
    QTest::newRow("testwebprompts") << formatUrl("testwebprompts.html") << 7 << 5 << 1 << 1;
}

void tst_webview::testLiveTabCount()
{
    QFETCH(QString, newUrl);
    QFETCH(int, expectedTabCount);
    QFETCH(int, liveTabCount);

    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));

    tabModel->newTab(newUrl);
    waitSignals(loadingChanged, 2);
    waitSignals(urlChangedSpy, 1);

    // New tab has always an empty title. Title of the container changes in case the title of
    // the page changes given that the page title differs.
    // So, number of change signals varies between 0-2 signals.
    QFETCH(int, willHaveTitle);
    QFETCH(int, hadTitle);
    int changeCount = willHaveTitle + hadTitle;
    waitSignals(titleChangedSpy, changeCount);

    // ~last in the sequence of adding a new tab.
    waitSignals(tabAddedSpy, 1);

    // Url and title signals emitted are only once.
    QCOMPARE(tabCountSpy.count(), 1);
    QCOMPARE(tabModel->count(), expectedTabCount);
    QCOMPARE(activeTabChangedSpy.count(), 1);

    QCOMPARE(webContainer->m_webPages->count(), liveTabCount);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), liveTabCount);
}

void tst_webview::load(QString url, bool expectTitleChange, int waitUrlChanges)
{
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));
    QSignalSpy painted(webContainer->webPage(), SIGNAL(firstPaint(int,int)));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    webContainer->webPage()->loadTab(url, false);
    waitSignals(urlChangedSpy, waitUrlChanges);

    if (expectTitleChange) {
        waitSignals(titleChangedSpy, 1);
    }
    waitSignals(loadingChanged, 2);
    waitSignals(painted, 1);
    QTest::qWait(100);
}

void tst_webview::goBack()
{
    QSignalSpy painted(webContainer->webPage(), SIGNAL(firstPaint(int,int)));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    webContainer->goBack();
    waitSignals(painted, 2);
    waitSignals(urlChangedSpy, 1);
}

void tst_webview::goForward()
{
    QSignalSpy painted(webContainer->webPage(), SIGNAL(firstPaint(int,int)));
    webContainer->goForward();
    waitSignals(painted, 2);
}

void tst_webview::forwardBackwardNavigation()
{
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy forwardSpy(webContainer, SIGNAL(canGoForwardChanged()));
    QSignalSpy backSpy(webContainer, SIGNAL(canGoBackChanged()));

    QString url = formatUrl("testwindowopen.html");
    QString title = "Test window opening";

    QVERIFY(webContainer->webPage());
    load(url, true /* expectTitleChange */);

    waitSignals(urlChangedSpy, 3, 500);
    waitSignals(titleChangedSpy, 1, 500);

    // Following url changes is somewhat unpredicable.
    // Hence, do not spy them. Url value is checked in any case.
    // QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(webContainer->url(), url);
    QCOMPARE(webContainer->title(), title);

    QCOMPARE(backSpy.count(), 1);
    QVERIFY(webContainer->canGoBack());
    goBack();

    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 2);
    int urlSpyCount = urlChangedSpy.count();
    QCOMPARE(titleChangedSpy.count(), 2);

    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Verify that spy counters will not update (1sec should be enough)
    QTest::qWait(500);
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), urlSpyCount);
    QCOMPARE(titleChangedSpy.count(), 2);

    goForward();

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 3);

    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    forwardSpy.clear();
    backSpy.clear();
    urlChangedSpy.clear();
    titleChangedSpy.clear();
    goBack();

    QTest::qWait(500);
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 1);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);

    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());
    url = formatUrl("testurlscheme.html");
    title = "TestUrlScheme";
    load(url, true /* expectTitleChange */);

    waitSignals(urlChangedSpy, 3, 500);
    waitSignals(titleChangedSpy, 2, 500);

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 2);

    QCOMPARE(webContainer->url(), url);
    QCOMPARE(webContainer->title(), title);
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    url = formatUrl("testuseragent.html");
    title = "TestUserAgent";
    load(url, true /* expectTitleChange */);

    waitSignals(urlChangedSpy, 5, 500);
    waitSignals(titleChangedSpy, 3, 500);

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 5);
    QCOMPARE(titleChangedSpy.count(), 3);
    QCOMPARE(webContainer->url(), url);
    QCOMPARE(webContainer->title(), title);
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    // testwebprompts.html, testurlscheme.html, testuseragent.html
    // Navigate twice back.
    goBack();

    waitSignals(urlChangedSpy, 6, 500);
    waitSignals(titleChangedSpy, 4, 500);

    QCOMPARE(forwardSpy.count(), 3);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 6);
    QCOMPARE(titleChangedSpy.count(), 4);
    QCOMPARE(webContainer->url(), formatUrl("testurlscheme.html"));
    QCOMPARE(webContainer->title(), QString("TestUrlScheme"));
    QVERIFY(webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Back to first page.
    goBack();

    waitSignals(urlChangedSpy, 7, 500);
    waitSignals(titleChangedSpy, 5, 500);

    QCOMPARE(forwardSpy.count(), 3);
    QCOMPARE(backSpy.count(), 3);
    QCOMPARE(urlChangedSpy.count(), 7);
    QCOMPARE(titleChangedSpy.count(), 5);
    QCOMPARE(webContainer->url(), formatUrl("testwebprompts.html"));
    QCOMPARE(webContainer->title(), QString("Test Web Prompts"));
    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    // Same as in previous case
    QCOMPARE(tabModel->count(), 7);
    QCOMPARE(webContainer->m_webPages->count(), webContainer->maxLiveTabCount());
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), webContainer->maxLiveTabCount());

    QTest::qWait(500);
}

void tst_webview::clear()
{
    QSignalSpy tabClosed(tabModel, SIGNAL(tabClosed(int)));
    historyModel->clear();
    tabModel->clear();
    QTest::qWait(500);
    waitSignals(tabClosed, 7);

    // From the previous case there should be 7 tabs to close
    QCOMPARE(tabClosed.count(), 7);
    QVERIFY(historyModel->rowCount() == 0);
    QVERIFY(webContainer->m_webPages->count() == 0);
    QVERIFY(webContainer->m_webPages->m_activePages.count() == 0);
    QVERIFY(!webContainer->m_webPages->m_activePages.activeWebPage());
    QVERIFY(!webContainer->m_webPage);

    QTest::qWait(500);
}

void tst_webview::restart()
{
    QSKIP("JB#57490 - Failing to restart, releasing of QMozWindow needs changes.");
    QSignalSpy historyAvailable(DBManager::instance(), SIGNAL(historyAvailable(QList<Link>)));

    // Title "TestPage"
    QString testPageUrl = formatUrl("testpage.html");
    tabModel->newTab(testPageUrl);
    load(testPageUrl, false /* expectTitleChange */, 1);
    waitSignals(historyAvailable, 1);
    historyAvailable.clear();

    QCOMPARE(tabModel->activeTab().url(), testPageUrl);
    QCOMPARE(webContainer->url(), testPageUrl);

    // Title "TestUserAgent"
    QString testUserAgentUrl = formatUrl("testuseragent.html");
    load(testUserAgentUrl, true /* expectTitleChange */);
    waitSignals(historyAvailable, 1);

    QCOMPARE(tabModel->activeTab().url(), testUserAgentUrl);
    QCOMPARE(webContainer->url(), testUserAgentUrl);
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(webContainer->m_webPages->count(), 1);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 1);

    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    QList<TestTab> historyOrder;
    historyOrder.append(TestTab(formatUrl("testuseragent.html"), QString("TestUserAgent")));
    historyOrder.append(TestTab(formatUrl("testpage.html"), QString("TestPage")));

    // Before restart
    QVERIFY(verifyHistory(historyOrder));

    tabModel->deleteLater();
    QTest::waitForEvents();

    // Fake closing the window.
    QEvent closeEvent(QEvent::Close);
    qGuiApp->sendEvent(webContainer, &closeEvent);
    QTest::waitForEvents();

    delete historyModel;
    historyModel = 0;
    delete webContainer;
    webContainer = 0;

    QTest::qWait(1000);

    setTestData(EMPTY_QML);
    setTestUrl(QUrl("qrc:///tst_webview.qml"));
    QTest::qWait(1000);

    webContainer = TestObject::qmlObject<DeclarativeWebContainer>("webView");
    QQmlEngine::setObjectOwnership(webContainer, QQmlEngine::CppOwnership);

    historyModel = TestObject::qmlObject<DeclarativeHistoryModel>("historyModel");
    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    tabModel->m_unittestMode = true;

    QTest::qWait(1000);

    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(tabModel->activeTab().url(), testUserAgentUrl);
    QCOMPARE(webContainer->url(), testUserAgentUrl);

    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());

    // After restart
    QVERIFY(verifyHistory(historyOrder));

    QSignalSpy urlChanged(webContainer, SIGNAL(urlChanged()));
    webContainer->goBack();
    urlChanged.wait();

    // After back navigation
    historyOrder.append(historyOrder.first());
    historyOrder.pop_front();
    QVERIFY(verifyHistory(historyOrder));

    QVERIFY(!webContainer->canGoBack());
    QVERIFY(webContainer->canGoForward());

    QCOMPARE(tabModel->activeTab().url(), testPageUrl);
    QCOMPARE(webContainer->url(), testPageUrl);
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(webContainer->m_webPages->count(), 1);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 1);
}

void tst_webview::changeTabAndLoad()
{
    QSKIP("JB#56602 - Fix skipped changeTabAndLoad from tst_webview, currently this crashes");

    int previousTab = tabModel->activeTab().tabId();
    tabModel->newTab(formatUrl("testwindowopen.html"));
    QTest::qWait(500);

    QList<TestTab> historyOrder;
    historyOrder.append(TestTab(formatUrl("testwindowopen.html"), QString("Test window opening")));
    historyOrder.append(TestTab(formatUrl("testpage.html"), QString("TestPage")));
    historyOrder.append(TestTab(formatUrl("testuseragent.html"), QString("TestUserAgent")));

    verifyHistory(historyOrder);

    // Active previous tab and an load url.
    tabModel->activateTabById(previousTab);
    QString testSelect(formatUrl("testselect.html"));
    load(testSelect, true /* expectTitleChange */);

    // Should be before "testpage.html"
    historyOrder.insert(0, TestTab(formatUrl("testselect.html"), QString("TestSelect")));
    verifyHistory(historyOrder);

    QCOMPARE(webContainer->url(), testSelect);
    QCOMPARE(webContainer->title(), QString("TestSelect"));
    QCOMPARE(tabModel->activeTab().url(), testSelect);
    QCOMPARE(tabModel->activeTab().title(), QString("TestSelect"));
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(webContainer->m_webPages->count(), 2);
    QCOMPARE(webContainer->m_webPages->m_activePages.count(), 2);
}

/*!
    Format url from \a fileName that is given as relative to the homePage.
*/
QString tst_webview::formatUrl(QString fileName) const
{
    return QUrl::fromLocalFile(baseUrl + "/" + fileName).toString();
}

bool tst_webview::verifyHistory(QList<TestTab> &historyOrder)
{
    historyModel->search("");
    QTest::qWait(1000);
    if (historyModel->rowCount() != historyOrder.count()) {
        return false;
    }
    bool found = false;
    for (const auto &testTab : historyOrder) {
        found = false;
        for (int i = 0; i < historyOrder.count(); ++i) {
            QModelIndex modelIndex = historyModel->createIndex(i, 0);
            QString url = historyModel->data(modelIndex, DeclarativeHistoryModel::UrlRole).toString();
            QString title = historyModel->data(modelIndex, DeclarativeHistoryModel::TitleRole).toString();
            if (testTab.url == url && testTab.title == title) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(new QGuiApplication(argc, argv));
    app->setQuitOnLastWindowClosed(false);

    const char *uri = "Sailfish.Browser";

    // Use QtQuick 2.1 for Sailfish.Browser imports
    qmlRegisterRevision<QQuickItem, 1>(uri, 1, 0);
    qmlRegisterRevision<QWindow, 1>(uri, 1, 0);

    qmlRegisterType<DeclarativeHistoryModel>(uri, 1, 0, "HistoryModel");
    qmlRegisterUncreatableType<DeclarativeTabModel>(uri, 1, 0, "TabModel", "TabModel is abstract!");
    qmlRegisterUncreatableType<PersistentTabModel>(uri, 1, 0, "PersistentTabModel", "");
    qmlRegisterType<DeclarativeWebContainer>(uri, 1, 0, "WebContainer");
    qmlRegisterType<DeclarativeWebPage>(uri, 1, 0, "WebPage");
    qmlRegisterType<DeclarativeWebPageCreator>(uri, 1, 0, "WebPageCreator");

    qmlRegisterUncreatableType<PrivateTabModel>(uri, 1, 0, "PrivateTabModel", "");
    qmlRegisterType<DeclarativeBookmarkModel>(uri, 1, 0, "BookmarkModel");
    qmlRegisterType<BookmarkFilterModel>(uri, 1, 0, "BookmarkFilterModel");
    qmlRegisterType<DeclarativeLoginModel>(uri, 1, 0, "LoginModel");
    qmlRegisterType<LoginFilterModel>(uri, 1, 0, "LoginFilterModel");
    qmlRegisterSingletonType<FaviconManager>(uri, 1, 0, "FaviconManager", faviconmanager_factory);
    qmlRegisterUncreatableType<DownloadStatus>(uri, 1, 0, "DownloadStatus", "");
    qmlRegisterType<DesktopBookmarkWriter>(uri, 1, 0, "DesktopBookmarkWriter");
    qmlRegisterType<DataFetcher>(uri, 1, 0, "DataFetcher");
    qmlRegisterType<InputRegion>(uri, 1, 0, "InputRegion");
    qmlRegisterSingletonType<SearchEngineModel>(uri, 1, 0, "SearchEngineModel", search_model_factory);

    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    qDebug() << "Profile path: " << path;
    SailfishOS::WebEngine::initialize(path);

    tst_webview *testcase = new tst_webview;

    testcase->setContextProperty("WebUtils", DeclarativeWebUtils::instance());
    testcase->setContextProperty("Settings", SettingManager::instance());

    QString mozillaDir = QString("%1/.mozilla/").arg(path);
    QDir dir(mozillaDir);
    if (dir.exists()) {
        // Remove any existing profile
        dir.removeRecursively();
    }
    dir.mkpath(dir.path());
    QFile::copy("/usr/share/sailfish-browser/data/prefs.js", mozillaDir + "prefs.js");

    bool contextDestroyed = false;

    QObject::connect(SailfishOS::WebEngine::instance(),
                     &SailfishOS::WebEngine::lastViewDestroyed,
                     [&] {
        if (testcase->running) {
            return;
        }

        QEvent closeEvent(QEvent::Close);
        qGuiApp->sendEvent(testcase->quickView(), &closeEvent);
        QTest::waitForEvents();
    });

    QObject::connect(SailfishOS::WebEngine::instance(),
                     &SailfishOS::WebEngine::lastWindowDestroyed,
                     [&] {
        if (testcase->running) {
            return;
        }

        SailfishOS::WebEngine::instance()->stopEmbedding();
        delete testcase->webContainer;
        testcase->webContainer = nullptr;
    });

    QObject::connect(SailfishOS::WebEngine::instance(),
                     &SailfishOS::WebEngine::contextDestroyed,
                     [&] {
        if (testcase->running) {
            return;
        }
        contextDestroyed = true;
    });

    int ret = QTest::qExec(testcase, argc, argv);
    testcase->running = false;

    bool hasTabs = testcase->tabModel->count() > 0;
    testcase->tabModel->clear();
    testcase->tabModel->deleteLater();
    if (!hasTabs) {
        QEvent closeEvent(QEvent::Close);
        qGuiApp->sendEvent(testcase->quickView(), &closeEvent);
        QTest::waitForEvents();
    }

    while (!contextDestroyed) {
        QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents;
        QCoreApplication::processEvents(flags, 500);
    }
    delete testcase;
    testcase = nullptr;

    QString dbFileName = QString("%1/%2")
            .arg(BrowserPaths::dataLocation())
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(dbFileName);
    dbFile.remove();

    return ret;
}

#include "tst_webview.moc"
