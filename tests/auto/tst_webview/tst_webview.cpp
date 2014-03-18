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
#include <quickmozview.h>

#include "declarativetab.h"
#include "declarativetabmodel.h"
#include "declarativewebcontainer.h"
#include "declarativewebviewcreator.h"
#include "qmozcontext.h"
#include "webUtilsMock.h"

class tst_webview : public QObject
{
    Q_OBJECT

public:
    tst_webview(QQuickView * view, QObject *parent = 0);

private slots:
    void initTestCase();
    void cleanupTestCase();

private:
    DeclarativeTabModel *tabModel;
    DeclarativeTab *tab;
    DeclarativeWebContainer *webContainer;
    QQuickView *view;
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

    var = webContainer->property("tabModel");
    tabModel = qobject_cast<DeclarativeTabModel *>(qvariant_cast<QObject*>(var));
    QVERIFY(tabModel);

    tab = webContainer->currentTab();
    QVERIFY(tab);

    QSignalSpy viewReady(webContainer, SIGNAL(_readyToLoadChanged()));
    QSignalSpy firstPageLoaded(webContainer, SIGNAL(loadedChanged()));
    viewReady.wait();
    firstPageLoaded.wait();

    QuickMozView *webView = webContainer->webView();
    QVERIFY(webView);
    QVERIFY(WebUtilsMock::instance());
    QCOMPARE(webView->url().toString(), WebUtilsMock::instance()->homePage);
    QCOMPARE(webView->title(), QString("TestPage"));
    QCOMPARE(tab->url(), WebUtilsMock::instance()->homePage);
    QCOMPARE(tab->title(), QString("TestPage"));
    QCOMPARE(tabModel->count(), 1);
}

void tst_webview::cleanupTestCase()
{
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
