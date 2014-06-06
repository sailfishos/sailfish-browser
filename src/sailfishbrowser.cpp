/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQml>
#include <QTimer>
#include <QTranslator>
#include <QDir>
#include <QScreen>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>

#include "qmozcontext.h"

#include "declarativebookmarkmodel.h"
#include "desktopbookmarkwriter.h"
#include "declarativewebutils.h"
#include "browserservice.h"
#include "downloadmanager.h"
#include "closeeventfilter.h"
#include "declarativetabmodel.h"
#include "declarativehistorymodel.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "declarativewebviewcreator.h"
#include "declarativefileuploadmode.h"
#include "declarativefileuploadfilter.h"

#ifdef HAS_BOOSTER
#include <MDeclarativeCache>
#endif

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    // EGL FPS are lower with threaded render loop
    // that's why this workaround.
    // See JB#7358
    setenv("QML_BAD_GUI_RENDER_LOOP", "1", 1);
    setenv("USE_ASYNC", "1", 1);

    // Workaround for https://bugzilla.mozilla.org/show_bug.cgi?id=929879
    setenv("LC_NUMERIC", "C", 1);
    setlocale(LC_NUMERIC, "C");
#ifdef HAS_BOOSTER
    QScopedPointer<QGuiApplication> app(MDeclarativeCache::qApplication(argc, argv));
    QScopedPointer<QQuickView> view(MDeclarativeCache::qQuickView());
#else
    QScopedPointer<QGuiApplication> app(new QGuiApplication(argc, argv));
    QScopedPointer<QQuickView> view(new QQuickView);
#endif
    app->setQuitOnLastWindowClosed(false);

    // GRE_HOME must be set before QMozContext is initialized.
    // With invoker PWD is empty.
    QByteArray binaryPath = QCoreApplication::applicationDirPath().toLocal8Bit();
    setenv("GRE_HOME", binaryPath.constData(), 1);

    // TODO : Remove this and set custom user agent always
    // Don't set custom user agent string when arguments contains -developerMode, give url as last argument
    if (!app->arguments().contains("-developerMode")) {
        setenv("CUSTOM_UA", "Mozilla/5.0 (Maemo; Linux; U; Jolla; Sailfish; Mobile; rv:29.0) Gecko/29.0 Firefox/29.0 SailfishBrowser/1.0", 1);
    }

    BrowserService *service = new BrowserService(app.data());
    // Handle command line launch
    if (!service->registered()) {

        QDBusMessage message;
        if (app->arguments().contains("-dumpMemory")) {
            int index = app->arguments().indexOf("-dumpMemory");
            QString fileName;
            if (index + 1 < app->arguments().size()) {
                fileName = app->arguments().at(index + 1);
            }

            message = QDBusMessage::createMethodCall(service->serviceName(), "/",
                                                     service->serviceName(), "dumpMemoryInfo");
            message.setArguments(QVariantList() << fileName);
        } else {
            message = QDBusMessage::createMethodCall(service->serviceName(), "/",
                                                     service->serviceName(), "openUrl");
            QStringList args;
            // Pass url argument if given
            if (app->arguments().count() > 1) {
                args << app->arguments().at(1);
            }
            message.setArguments(QVariantList() << args);
        }

        QDBusConnection::sessionBus().asyncCall(message);
        if (QCoreApplication::hasPendingEvents()) {
            QCoreApplication::processEvents();
        }

        return 0;
    }

    QString translationPath("/usr/share/translations/");
    QTranslator engineeringEnglish;
    engineeringEnglish.load("sailfish-browser_eng_en", translationPath);
    qApp->installTranslator(&engineeringEnglish);

    QTranslator translator;
    translator.load(QLocale(), "sailfish-browser", "-", translationPath);
    qApp->installTranslator(&translator);

    //% "Browser"
    view->setTitle(qtTrId("sailfish-browser-ap-name"));

    qmlRegisterType<DeclarativeBookmarkModel>("Sailfish.Browser", 1, 0, "BookmarkModel");
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    qmlRegisterType<DeclarativeHistoryModel>("Sailfish.Browser", 1, 0, "HistoryModel");
    qmlRegisterType<DeclarativeWebContainer>("Sailfish.Browser", 1, 0, "WebContainer");
    qmlRegisterType<DeclarativeWebPage>("Sailfish.Browser", 1, 0, "WebPage");
    qmlRegisterType<DeclarativeWebViewCreator>("Sailfish.Browser", 1, 0, "WebViewCreator");
    qmlRegisterType<DeclarativeFileUploadMode>("Sailfish.Browser", 1, 0, "FileUploadMode");
    qmlRegisterType<DeclarativeFileUploadFilter>("Sailfish.Browser", 1, 0, "FileUploadFilter");
    qmlRegisterType<DesktopBookmarkWriter>("Sailfish.Browser", 1, 0, "DesktopBookmarkWriter");

    QString componentPath(DEFAULT_COMPONENTS_PATH);
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteBinComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/components/EmbedLiteJSComponents.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteJSScripts.manifest"));
    QMozContext::GetInstance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteOverrides.manifest"));

    app->setApplicationName(QString("sailfish-browser"));
    app->setOrganizationName(QString("org.sailfishos"));

    DeclarativeWebUtils *utils = DeclarativeWebUtils::instance();
    utils->connect(service, SIGNAL(openUrlRequested(QString)),
                   utils, SIGNAL(openUrlRequested(QString)));
    utils->connect(service, SIGNAL(dumpMemoryInfoRequested(QString)),
                   utils, SLOT(handleDumpMemoryInfoRequest(QString)));

    utils->clearStartupCacheIfNeeded();
    view->rootContext()->setContextProperty("WebUtils", utils);
    view->rootContext()->setContextProperty("MozContext", QMozContext::GetInstance());
    view->rootContext()->setContextProperty("Settings", SettingManager::instance());

    DownloadManager *dlMgr = DownloadManager::instance();
    dlMgr->connect(service, SIGNAL(cancelTransferRequested(int)),
            dlMgr, SLOT(cancelTransfer(int)));
    dlMgr->connect(service, SIGNAL(restartTransferRequested(int)),
            dlMgr, SLOT(restartTransfer(int)));

    CloseEventFilter * clsEventFilter = new CloseEventFilter(dlMgr, app.data());
    view->installEventFilter(clsEventFilter);
    QObject::connect(service, SIGNAL(openUrlRequested(QString)),
                     clsEventFilter, SLOT(cancelStopApplication()));

#ifdef USE_RESOURCES
    view->setSource(QUrl("qrc:///browser.qml"));
#else
    bool isDesktop = qApp->arguments().contains("-desktop");

    QString path;
    if (isDesktop) {
        path = qApp->applicationDirPath() + QDir::separator();
    } else {
        path = QString(DEPLOYMENT_PATH);
    }
    view->setSource(QUrl::fromLocalFile(path+"browser.qml"));
#endif
    // ### Temporary solution, the rendering engine should handle QQuickWindow::sceneGraphInvalidated()
    view->setPersistentOpenGLContext(true);
    view->setPersistentSceneGraph(true);

    view->showFullScreen();

    // Setup embedding
    QTimer::singleShot(0, QMozContext::GetInstance(), SLOT(runEmbedding()));

    if (qApp->arguments().count() > 1) {
        emit utils->openUrlRequested(qApp->arguments().last());
    }

    return app->exec();
}
