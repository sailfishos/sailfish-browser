/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
** Copyright (c) 2021 Jolla Ltd.
** Copyright (c) 2026 Jolla Mobile Ltd
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QGuiApplication>
#include <QQuickView>
#include <qqmldebug.h>
#include <QtQml>
#include <QTranslator>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>

#include "browser.h"
#include "externalurlhandler.h"
#include "captiveportalservice.h"
#include "browserservice.h"
#include "browserappinfo.h"
// Registered QML types
#include "downloadstatus.h"
#include "persistenttabmodel.h"
#include "privatetabmodel.h"
#include "declarativehistorymodel.h"
#include "declarativewebcontainer.h"
#include <qmoznativeview.h>
#include "inputregion.h"

#ifdef HAS_BOOSTER
#include <MDeclarativeCache>
#endif

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    // Disable crash guard and prefer egl for webgl.
    setenv("MOZ_DISABLE_CRASH_GUARD", "1", 1);
    setenv("MOZ_WEBGL_PREFER_EGL", "1", 1);

    QQuickWindow::setDefaultAlphaBuffer(true);

    if (!qgetenv("QML_DEBUGGING_ENABLED").isEmpty()) {
        QQmlDebuggingEnabler qmlDebuggingEnabler;
    }

#ifdef HAS_BOOSTER
    QScopedPointer<QGuiApplication> app(MDeclarativeCache::qApplication(argc, argv));
    QScopedPointer<QQuickView> view(MDeclarativeCache::qQuickView());
#else
    QScopedPointer<QGuiApplication> app(new QGuiApplication(argc, argv));
    QScopedPointer<QQuickView> view(new QQuickView);
#endif
    app->setQuitOnLastWindowClosed(false);
    app->setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);

    const bool sparse = BrowserAppInfo::sparse();
    app->setApplicationName(sparse ? QStringLiteral("browser-sparse") : QStringLiteral("captiveportal"));
    app->setOrganizationName(QStringLiteral("org.sailfishos"));

    CaptivePortalService *portalService = sparse ? nullptr : new CaptivePortalService(app.data());
    BrowserService *browserService = sparse ? new BrowserService(app.data()) : nullptr;
    const QString serviceName = sparse ? browserService->serviceName() : portalService->serviceName();
    const bool registered = sparse ? browserService->registered() : portalService->registered();
    if (portalService) {
        QObject::connect(portalService, &CaptivePortalService::closeBrowserRequested,
                         view.data(), &QWindow::close);
    }
    // Handle command line launch
    if (!registered) {
        QDBusMessage message = QDBusMessage::createMethodCall(serviceName, "/",
                                                              serviceName, "openUrl");
        QStringList args;
        // Pass url argument if given
        for (const QString &argument : app->arguments().mid(1)) {
            if (!argument.startsWith(QLatin1Char('-'))) args << argument;
        }
        message.setArguments(QVariantList() << args);

        auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), app.data());
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, app.data(), &QCoreApplication::quit);
        app->exec();

        return 0;
    }

    QString translationPath("/usr/share/translations/");
    QTranslator engineeringEnglish;
    engineeringEnglish.load("sailfish-captiveportal_eng_en", translationPath);
    qApp->installTranslator(&engineeringEnglish);

    QTranslator translator;
    translator.load(QLocale(), "sailfish-captiveportal", "-", translationPath);
    qApp->installTranslator(&translator);

    //% "Network login portal"
    view->setTitle(qtTrId("sailfish-captiveportal-ap-name"));

    QTranslator sharedEnglish;
    sharedEnglish.load("sailfish-browser_eng_en", translationPath);
    app->installTranslator(&sharedEnglish);
    QTranslator sharedTranslator;
    sharedTranslator.load(QLocale(), "sailfish-browser", "-", translationPath);
    app->installTranslator(&sharedTranslator);

    const char *uri = "Sailfish.Browser";

    // Use QtQuick 2.1 for Sailfish.Browser imports
    qmlRegisterRevision<QQuickItem, 1>(uri, 1, 0);
    qmlRegisterRevision<QWindow, 1>(uri, 1, 0);

    qmlRegisterUncreatableType<DeclarativeTabModel>(uri, 1, 0, "TabModel", "TabModel is abstract!");
    qmlRegisterUncreatableType<PrivateTabModel>(uri, 1, 0, "PrivateTabModel", "");

    qmlRegisterUncreatableType<DownloadStatus>(uri, 1, 0, "DownloadStatus", "");
    qmlRegisterType<DeclarativeWebContainer>(uri, 1, 0, "WebContainer");
    if (DeclarativeWebContainer::nativePresentationEnabled()) {
        qmlRegisterType<QMozNativeView>(uri, 1, 0, "BrowserContentView");
    } else {
        qmlRegisterType<QuickMozView>(uri, 1, 0, "BrowserContentView");
    }
    qmlRegisterType<ExternalUrlHandler>(uri, 1, 0, "ExternalUrlHandler");
    qmlRegisterType<InputRegion>(uri, 1, 0, "InputRegion");

    Browser *browser = new Browser(view.data(), DEPLOYMENT_PATH, app.data());
    if (portalService) {
        QObject::connect(portalService, &CaptivePortalService::openUrlRequested, browser, &Browser::openUrl);
        QObject::connect(portalService, &CaptivePortalService::cancelTransferRequested, browser, &Browser::cancelDownload);
        QObject::connect(portalService, &CaptivePortalService::restartTransferRequested, browser, &Browser::restartDownload);
    } else {
        auto *uiService = new BrowserUIService(app.data());
        QObject::connect(browserService, &BrowserService::openUrlRequested, browser, &Browser::openUrl);
        QObject::connect(uiService, &BrowserUIService::openUrlRequested, browser, &Browser::openUrl);
        QObject::connect(uiService, &BrowserUIService::showChrome, browser, &Browser::showChrome);
        QObject::connect(browserService, &BrowserService::cancelTransferRequested, browser, &Browser::cancelDownload);
        QObject::connect(browserService, &BrowserService::restartTransferRequested, browser, &Browser::restartDownload);
    }
    browser->load();
    return app->exec();
}
