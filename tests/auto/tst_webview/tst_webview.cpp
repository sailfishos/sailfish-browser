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
#include <QQmlContext>
#include <QQuickView>
#include <qmozcontext.h>

#include "declarativetab.h"
#include "declarativetabmodel.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "declarativewebviewcreator.h"
#include "webUtilsMock.h"

class tst_webview : public QObject
{
    Q_OBJECT

public:
    tst_webview(QQuickView * view, QObject *parent = 0);

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
    void cleanupTestCase();

private:
    QString formatUrl(QString fileName) const;
    void waitSignals(QSignalSpy &spy, int expectedSignalCount) const;

    DeclarativeTabModel *tabModel;
    DeclarativeTab *tab;
    DeclarativeWebContainer *webContainer;
    QQuickView *view;
    QString baseUrl;
};


tst_webview::tst_webview(QQuickView *view, QObject *parent)
    : QObject(parent)
    , tabModel(0)
    , tab(0)
    , view(view)
{
}

void tst_webview::initTestCase()
{
    view->setSource(QUrl("qrc:///tst_webview.qml"));
    view->showFullScreen();
    QTest::qWaitForWindowExposed(view);

    QQuickItem *appWindow = view->rootObject();
    QVariant var = appWindow->property("webView");
    webContainer = qobject_cast<DeclarativeWebContainer *>(qvariant_cast<QObject*>(var));
    QVERIFY(webContainer);
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));

    var = webContainer->property("tabModel");
    tabModel = qobject_cast<DeclarativeTabModel *>(qvariant_cast<QObject*>(var));
    QVERIFY(tabModel);
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));

    tab = webContainer->currentTab();
    QVERIFY(tab);

    waitSignals(loadingChanged, 2);

    DeclarativeWebPage *webPage = webContainer->webPage();
    QVERIFY(webPage);
    QVERIFY(WebUtilsMock::instance());
    QCOMPARE(webPage->url().toString(), WebUtilsMock::instance()->homePage);
    QCOMPARE(webPage->title(), QString("TestPage"));
    QCOMPARE(tab->url(), WebUtilsMock::instance()->homePage);
    QCOMPARE(tab->title(), QString("TestPage"));
    QCOMPARE(tabModel->count(), 1);

    baseUrl = QUrl(WebUtilsMock::instance()->homePage).toLocalFile();
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

    QString homePage = WebUtilsMock::instance()->homePage;
    QStringList activeTabOrder = QStringList() << formatUrl("testselect.html")
                                               << homePage;
    QTest::newRow("testselect") << formatUrl("testselect.html") << "TestSelect"
                                << "TestSelect" << 1 << activeTabOrder;

    activeTabOrder = QStringList() << formatUrl("testuseragent.html")
                                   << formatUrl("testselect.html")
                                   << homePage;
    QTest::newRow("testuseragent") << formatUrl("testuseragent.html") << "TestUserAgent"
                                   << "TestUserAgent" << 1 << activeTabOrder;

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
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
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

    // Url and title signals emitted are only once.
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), expectedTitleChangeCount);
    QCOMPARE(tabCountSpy.count(), 1);
    QCOMPARE(tabModel->count(), expectedTabCount);
    QCOMPARE(webContainer->url(), newUrl);
    QCOMPARE(webContainer->title(), expectedTitle);
    QCOMPARE(tab->url(), newUrl);
    QCOMPARE(tab->title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), 1);

    // Signaled always when tab is changed.
    QList<QVariant> arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(0).toInt();
    QCOMPARE(activatedTabId, tab->tabId());

    // Signaled only when tab added.
    QCOMPARE(tabAddedSpy.count(), 1);
    arguments = tabAddedSpy.takeFirst();
    int addedTabId = arguments.at(0).toInt();
    QCOMPARE(addedTabId, tab->tabId());

    QFETCH(QStringList, activeTabs);
    QCOMPARE(tab->url(), activeTabs.at(0));
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

    // "testpage.html", "TestPage"
    QModelIndex modelIndex = tabModel->createIndex(2, 0);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
    QString newActiveTitle = tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString();
    int newActiveTabId = tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt();

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy tabUrlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy tabTitleChangedSpy(tab, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    tabModel->activateTab(2);
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(contentItemSpy.count(), 1);
    // Tab already loaded.
    QVERIFY(!webContainer->webPage()->loading());
    QCOMPARE(webContainer->webPage()->loadProgress(), 100);

    QCOMPARE(tab->tabId(), newActiveTabId);
    QCOMPARE(webContainer->url(), newActiveUrl);
    QCOMPARE(webContainer->title(), newActiveTitle);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(tab->url(), newActiveUrl);
    QCOMPARE(tab->title(), newActiveTitle);
    QCOMPARE(tabUrlChangedSpy.count(), 1);
    QCOMPARE(tabTitleChangedSpy.count(), 1);

    // Signaled always when tab is changed.
    QList<QVariant> arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(0).toInt();
    QCOMPARE(activatedTabId, tab->tabId());
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

    int previousTabId = tab->tabId();

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));
    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy tabUrlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy tabTitleChangedSpy(tab, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    tabModel->closeActiveTab();
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(tabClosedSpy.count(), 1);
    QList<QVariant> arguments = tabClosedSpy.takeFirst();
    int closedTabId = arguments.at(0).toInt();
    QCOMPARE(closedTabId, previousTabId);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(contentItemSpy.count(), 1);
    // Tab already loaded.
    QVERIFY(!webContainer->webPage()->loading());
    QCOMPARE(webContainer->webPage()->loadProgress(), 100);

    QCOMPARE(tab->tabId(), newActiveTabId);
    QCOMPARE(webContainer->url(), newActiveUrl);
    QCOMPARE(webContainer->title(), newActiveTitle);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(tab->url(), newActiveUrl);
    QCOMPARE(tab->title(), newActiveTitle);
    QCOMPARE(tabUrlChangedSpy.count(), 1);
    QCOMPARE(tabTitleChangedSpy.count(), 1);

    // Signaled always when tab is changed.
    arguments = activeTabChangedSpy.takeFirst();
    int activatedTabId = arguments.at(0).toInt();
    QCOMPARE(activatedTabId, tab->tabId());
}

void tst_webview::testRemoveTab()
{
    // Active tabs in order:
    // "testnavigation.html", "TestNavigation" (active)
    // "testuseragent.html", "TestUserAgent" (0)
    // "testselect.html", "TestSelect" (1)

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));

    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy tabUrlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy tabTitleChangedSpy(tab, SIGNAL(titleChanged()));
    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));

    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    int tabIdOfIndexZero = tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt();
    tabModel->remove(0);

    QCOMPARE(tabClosedSpy.count(), 1);
    QList<QVariant> arguments = tabClosedSpy.takeFirst();
    int closedTabId = arguments.at(0).toInt();
    QCOMPARE(closedTabId, tabIdOfIndexZero);
    QCOMPARE(tabModel->count(), 2);

    QVERIFY(activeTabChangedSpy.count() == 0);
    QVERIFY(urlChangedSpy.count() == 0);
    QVERIFY(titleChangedSpy.count() == 0);
    QVERIFY(tabUrlChangedSpy.count() == 0);
    QVERIFY(tabTitleChangedSpy.count() == 0);
    QVERIFY(contentItemSpy.count() == 0);
}

void tst_webview::testUrlLoading()
{
    // Active tabs in order:
    // "testnavigation.html", "TestNavigation" (active)
    // "testselect.html", "TestSelect" (0)

    QSignalSpy urlChangedSpy(webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(webContainer, SIGNAL(titleChanged()));
    QSignalSpy tabUrlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy tabTitleChangedSpy(tab, SIGNAL(titleChanged()));
    QSignalSpy pageUrlChangedSpy(webContainer->webPage(), SIGNAL(urlChanged()));
    QSignalSpy pageTitleChangedSpy(webContainer->webPage(), SIGNAL(titleChanged()));
    QSignalSpy backNavigationChangedSpy(webContainer, SIGNAL(canGoBackChanged()));


    QSignalSpy contentItemSpy(webContainer, SIGNAL(contentItemChanged()));
    QSignalSpy loadingChanged(webContainer, SIGNAL(loadingChanged()));

    QString newUrl = formatUrl("testurlscheme.html");
    QString newTitle = "TestUrlScheme";

    // Mimic favorite opening to a new tab. Favorites can have both url and title and when entering
    // url through virtual keyboard only url is provided.
    QMetaObject::invokeMethod(webContainer, "load", Qt::DirectConnection,
                              Q_ARG(QVariant, newUrl),
                              Q_ARG(QVariant, newTitle),
                              Q_ARG(QVariant, false));
    waitSignals(loadingChanged, 2);

    QCOMPARE(contentItemSpy.count(), 0);

    QCOMPARE(pageUrlChangedSpy.count(), 1);
    QCOMPARE(pageTitleChangedSpy.count(), 1);
    QCOMPARE(webContainer->webPage()->url().toString(), newUrl);
    QCOMPARE(webContainer->webPage()->title(), newTitle);

    // Title changes twice as as url change will not trigger title change.
    // TODO: titleChangedSpy and tabTitleChangedSpy should be changed so
    // that emit happens only once (JB#16850).
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 3);
    QCOMPARE(webContainer->url(), newUrl);
    QCOMPARE(webContainer->title(), newTitle);
    QCOMPARE(tabUrlChangedSpy.count(), 1);
    QCOMPARE(tabTitleChangedSpy.count(), 3);
    QCOMPARE(tab->url(), newUrl);
    QCOMPARE(tab->title(), newTitle);

    waitSignals(backNavigationChangedSpy, 1);
    QVERIFY(webContainer->canGoBack());
    QVERIFY(!webContainer->canGoForward());
    QCOMPARE(tabModel->count(), 2);
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
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int)));
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
}

void tst_webview::cleanupTestCase()
{
    QTest::qWait(500);

    tabModel->clear();
    QVERIFY(tabModel->count() == 0);
    QVERIFY(tab->url().isEmpty());
    QVERIFY(tab->title().isEmpty());
    QVERIFY(!tab->valid());

    // Wait for event loop of db manager
    QTest::qWait(500);
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

/*!
    Wait signal of \a spy to be emitted \a expectedSignalCount.

    Note: this might cause indefinite loop, if not used cautiously. Check
    that \a spy is initialized before expected emits can happen.
 */
void tst_webview::waitSignals(QSignalSpy &spy, int expectedSignalCount) const
{
    while (spy.count() < expectedSignalCount) {
        spy.wait();
    }
}

int main(int argc, char *argv[])
{
    setenv("USE_ASYNC", "1", 1);
    setenv("QML_BAD_GUI_RENDER_LOOP", "1", 1);

    QGuiApplication app(argc, argv);
    QQuickView view;
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_webview testcase(&view);

    qmlRegisterUncreatableType<DeclarativeTab>("Sailfish.Browser", 1, 0, "Tab", "");
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    qmlRegisterType<DeclarativeWebContainer>("Sailfish.Browser", 1, 0, "WebContainer");
    qmlRegisterType<DeclarativeWebPage>("Sailfish.Browser", 1, 0, "WebPage");
    qmlRegisterType<DeclarativeWebViewCreator>("Sailfish.Browser", 1, 0, "WebViewCreator");
    qmlRegisterSingletonType<WebUtilsMock>("Sailfish.Browser", 1, 0, "WebUtils", WebUtilsMock::singletonApiFactory);
    view.rootContext()->setContextProperty("MozContext", QMozContext::GetInstance());

    QString componentPath(DEFAULT_COMPONENTS_PATH);
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteBinComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteJSComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteJSScripts.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteOverrides.manifest"));

    QTimer::singleShot(0, QMozContext::GetInstance(), SLOT(runEmbedding()));

    return QTest::qExec(&testcase, argc, argv);
}

#include "tst_webview.moc"
