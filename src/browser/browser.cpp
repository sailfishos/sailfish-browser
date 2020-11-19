/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "browser.h"
#include "browser_p.h"
#include "closeeventfilter.h"
#include "declarativewebutils.h"
#include "downloadmanager.h"
#include "settingmanager.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QTimer>
#include <QUrl>
#include <webengine.h>
#include <webenginesettings.h>

BrowserPrivate::BrowserPrivate(QQuickView *view)
    : view(view)
    , closeEventFilter(nullptr)
{
}

Browser::Browser(QQuickView *view, QObject *parent)
    : QObject(parent)
    , d_ptr(new BrowserPrivate(view))
{
    Q_D(Browser);

    Q_ASSERT(view);
    Q_ASSERT(qGuiApp);

    SailfishOS::WebEngine::initialize("mozembed");
    SailfishOS::WebEngineSettings::initialize();

    DeclarativeWebUtils *utils = DeclarativeWebUtils::instance();
    DownloadManager *downloadManager = DownloadManager::instance();

    d->view->rootContext()->setContextProperty("WebUtils", utils);
    d->view->rootContext()->setContextProperty("Settings", SettingManager::instance());
    d->view->rootContext()->setContextProperty("DownloadManager", downloadManager);

    d->closeEventFilter = new CloseEventFilter(downloadManager, this);
    d->view->installEventFilter(d->closeEventFilter);

#ifdef USE_RESOURCES
    d->view->setSource(QUrl("qrc:///browser.qml"));
#else
    d->view->setSource(QUrl::fromLocalFile(Browser::applicationFilePath() + "browser.qml"));
#endif
}

void Browser::load()
{
    Q_ASSERT_X(qGuiApp, Q_FUNC_INFO, "There should always be a QGuiApplication running.");
    const QStringList arguments = qGuiApp->arguments();
    if (!arguments.contains(QStringLiteral("-prestart"))) {
        if (arguments.count() > 1 && (arguments.last() != QStringLiteral("-debugMode"))) {
            DeclarativeWebUtils::instance()->openUrl(arguments.last());
        } else if (!DeclarativeWebUtils::instance()->firstUseDone()) {
            DeclarativeWebUtils::instance()->openUrl("");
        }
    }
}

QString Browser::applicationFilePath()
{
    Q_ASSERT_X(qGuiApp, Q_FUNC_INFO, "There should always be a QGuiApplication running.");
    if (qGuiApp->arguments().contains("-desktop")) {
        return qGuiApp->applicationDirPath() + QDir::separator();
    } else {
        return QString(DEPLOYMENT_PATH);
    }
}

void Browser::openUrl(const QString &url)
{
    Q_D(Browser);
    d->closeEventFilter->cancelStopApplication();
    DeclarativeWebUtils::instance()->openUrl(url);
}

void Browser::openNewTabView()
{
    Q_D(Browser);
    d->closeEventFilter->cancelStopApplication();
    emit DeclarativeWebUtils::instance()->activateNewTabViewRequested();
}

void Browser::showChrome()
{
    Q_D(Browser);
    d->closeEventFilter->cancelStopApplication();
    emit DeclarativeWebUtils::instance()->showChrome();
}

void Browser::cancelDownload(int transferId)
{
    DownloadManager::instance()->cancelTransfer(transferId);
}

void Browser::restartDownload(int transferid)
{
    DownloadManager::instance()->restartTransfer(transferid);
}

void Browser::dumpMemoryInfo(const QString &fileName)
{
    DeclarativeWebUtils::instance()->handleDumpMemoryInfoRequest(fileName);
}
