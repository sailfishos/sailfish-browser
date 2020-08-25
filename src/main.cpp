/****************************************************************************
**
** Copyright (C) 2013-2016 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
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

#include "browser.h"
#include "browserapp.h"
// Registered QML types
#include "declarativebookmarkmodel.h"
#include "desktopbookmarkwriter.h"
#include "downloadstatus.h"
#include "browserservice.h"
#include "persistenttabmodel.h"
#include "privatetabmodel.h"
#include "declarativehistorymodel.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "declarativewebpagecreator.h"
#include "iconfetcher.h"
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

    BrowserUIService *uiService = nullptr;
    if (!BrowserApp::captivePortal())
        uiService = new BrowserUIService(app.data());

    QString translationPath("/usr/share/translations/");
    QTranslator engineeringEnglish;
    engineeringEnglish.load("sailfish-browser_eng_en", translationPath);
    qApp->installTranslator(&engineeringEnglish);

    QTranslator translator;
    translator.load(QLocale(), "sailfish-browser", "-", translationPath);
    qApp->installTranslator(&translator);

    //% "Browser"
    view->setTitle(qtTrId("sailfish-browser-ap-name"));

    if (BrowserApp::captivePortal())
        app->setApplicationName(QStringLiteral("sailfish-browser-captiveportal"));
    else
        app->setApplicationName(QStringLiteral("sailfish-browser"));
    app->setOrganizationName(QStringLiteral("org.sailfishos"));

    const char *uri = "Sailfish.Browser";

    // Use QtQuick 2.1 for Sailfish.Browser imports
    qmlRegisterRevision<QQuickItem, 1>(uri, 1, 0);
    qmlRegisterRevision<QWindow, 1>(uri, 1, 0);

    qmlRegisterType<DeclarativeBookmarkModel>(uri, 1, 0, "BookmarkModel");
    qmlRegisterUncreatableType<DeclarativeTabModel>(uri, 1, 0, "TabModel", "TabModel is abstract!");
    qmlRegisterUncreatableType<PersistentTabModel>(uri, 1, 0, "PersistentTabModel", "");
    qmlRegisterUncreatableType<PrivateTabModel>(uri, 1, 0, "PrivateTabModel", "");
    qmlRegisterUncreatableType<DownloadStatus>(uri, 1, 0, "DownloadStatus", "");
    qmlRegisterType<DeclarativeHistoryModel>(uri, 1, 0, "HistoryModel");
    qmlRegisterType<DeclarativeWebContainer>(uri, 1, 0, "WebContainer");
    qmlRegisterType<DeclarativeWebPage>(uri, 1, 0, "WebPage");
    qmlRegisterType<DeclarativeWebPageCreator>(uri, 1, 0, "WebPageCreator");
    qmlRegisterType<DesktopBookmarkWriter>(uri, 1, 0, "DesktopBookmarkWriter");
    qmlRegisterType<IconFetcher>(uri, 1, 0, "IconFetcher");
    qmlRegisterType<InputRegion>(uri, 1, 0, "InputRegion");

    Browser *browser = new Browser(view.data(), app.data());
    browser->connect(service, &BrowserService::openUrlRequested,
                     browser, &Browser::openUrl);
    browser->connect(service, &BrowserService::activateNewTabViewRequested,
                     browser, &Browser::openNewTabView);
    browser->connect(service, &BrowserService::dumpMemoryInfoRequested,
                     browser, &Browser::dumpMemoryInfo);

    if (uiService)
    {
        browser->connect(uiService, &BrowserUIService::openUrlRequested,
                        browser, &Browser::openUrl);
        browser->connect(uiService, &BrowserUIService::activateNewTabViewRequested,
                        browser, &Browser::openNewTabView);
        browser->connect(uiService, &BrowserUIService::showChrome,
                        browser, &Browser::showChrome);
    }

    browser->connect(service, &BrowserService::cancelTransferRequested,
                     browser, &Browser::cancelDownload);
    browser->connect(service, &BrowserService::restartTransferRequested,
                     browser, &Browser::restartDownload);
    browser->load();
    return app->exec();
}
