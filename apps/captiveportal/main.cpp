/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
** Copyright (c) 2021 Jolla Ltd.
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
#include "captiveportalservice.h"
// Registered QML types
#include "downloadstatus.h"
#include "persistenttabmodel.h"
#include "privatetabmodel.h"
#include "declarativehistorymodel.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "declarativewebpagecreator.h"
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

    CaptivePortalService *service = new CaptivePortalService(app.data());
    // Handle command line launch
    if (!service->registered()) {
        QDBusMessage message = QDBusMessage::createMethodCall(service->serviceName(), "/",
                                                              service->serviceName(), "openUrl");
        QStringList args;
        // Pass url argument if given
        if (app->arguments().count() > 1) {
            args << app->arguments().at(1);
        }
        message.setArguments(QVariantList() << args);

        QDBusConnection::sessionBus().asyncCall(message);
        if (QCoreApplication::hasPendingEvents()) {
            QCoreApplication::processEvents();
        }

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

    app->setApplicationName(QStringLiteral("captiveportal"));
    app->setOrganizationName(QStringLiteral("org.sailfishos"));

    const char *uri = "Sailfish.Browser";

    // Use QtQuick 2.1 for Sailfish.Browser imports
    qmlRegisterRevision<QQuickItem, 1>(uri, 1, 0);
    qmlRegisterRevision<QWindow, 1>(uri, 1, 0);

    qmlRegisterUncreatableType<DeclarativeTabModel>(uri, 1, 0, "TabModel", "TabModel is abstract!");
    qmlRegisterUncreatableType<PrivateTabModel>(uri, 1, 0, "PrivateTabModel", "");

    qmlRegisterUncreatableType<DownloadStatus>(uri, 1, 0, "DownloadStatus", "");
    qmlRegisterType<DeclarativeWebContainer>(uri, 1, 0, "WebContainer");
    qmlRegisterType<DeclarativeWebPage>(uri, 1, 0, "WebPage");
    qmlRegisterType<DeclarativeWebPageCreator>(uri, 1, 0, "WebPageCreator");
    qmlRegisterType<InputRegion>(uri, 1, 0, "InputRegion");

    Browser *browser = new Browser(view.data(), app.data());
    browser->connect(service, &CaptivePortalService::openUrlRequested,
                     browser, &Browser::openUrl);
    browser->load();
    return app->exec();
}
