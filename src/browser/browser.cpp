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
#include <qmozcontext.h>

BrowserPrivate::BrowserPrivate(QQuickView *view)
    : view(view)
    , closeEventFilter(0)
{
}

Browser::Browser(QQuickView *view, QObject *parent)
    : QObject(parent)
    , d_ptr(new BrowserPrivate(view))
{
    Q_D(Browser);

    Q_ASSERT(view);
    Q_ASSERT(qGuiApp);

    setenv("USE_ASYNC", "1", 1);
    setenv("USE_NEMO_GSTREAMER", "1", 1);
    setenv("NO_LIMIT_ONE_GST_DECODER", "1", 1);

    // See https://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/ApplicationProperties/
    setenv("PULSE_PROP_application.process.binary", "sailfish-browser", 1);

    // Workaround for https://bugzilla.mozilla.org/show_bug.cgi?id=929879
    setenv("LC_NUMERIC", "C", 1);
    setlocale(LC_NUMERIC, "C");

    // GRE_HOME must be set before QMozContext is initialized.
    // With invoker PWD is empty.
    QByteArray binaryPath = QCoreApplication::applicationDirPath().toLocal8Bit();
    setenv("GRE_HOME", binaryPath.constData(), 1);

    // Don't set custom user agent string when the environment already contains CUSTOM_UA.
    if (qgetenv("CUSTOM_UA").isEmpty()) {
        setenv("CUSTOM_UA", "Mozilla/5.0 (Maemo; Linux; U; Jolla; Sailfish; Mobile; rv:38.0) Gecko/38.0 Firefox/38.0 SailfishBrowser/1.0", 1);
    }

    QString componentPath(DEFAULT_COMPONENTS_PATH);
    QMozContext::instance()->addComponentManifest(componentPath + QString("/components/EmbedLiteBinComponents.manifest"));
    QMozContext::instance()->addComponentManifest(componentPath + QString("/components/EmbedLiteJSComponents.manifest"));
    QMozContext::instance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteJSScripts.manifest"));
    QMozContext::instance()->addComponentManifest(componentPath + QString("/chrome/EmbedLiteOverrides.manifest"));

    DeclarativeWebUtils *utils = DeclarativeWebUtils::instance();
    DownloadManager *downloadManager = DownloadManager::instance();

    utils->clearStartupCacheIfNeeded();

    d->view->rootContext()->setContextProperty("WebUtils", utils);
    d->view->rootContext()->setContextProperty("MozContext", QMozContext::instance());
    d->view->rootContext()->setContextProperty("Settings", SettingManager::instance());
    d->view->rootContext()->setContextProperty("DownloadManager", downloadManager);

    d->closeEventFilter = new CloseEventFilter(downloadManager, this);
    d->view->installEventFilter(d->closeEventFilter);

#ifdef USE_RESOURCES
    d->view->setSource(QUrl("qrc:///browser.qml"));
#else
    d->view->setSource(QUrl::fromLocalFile(Browser::applicationFilePath() + "browser.qml"));
#endif

    // Setup embedding
    QTimer::singleShot(0, QMozContext::instance(), SLOT(runEmbedding()));
}

void Browser::load()
{
    Q_ASSERT_X(qGuiApp, Q_FUNC_INFO, "There should always be a QGuiApplication running.");
    const QStringList arguments = qGuiApp->arguments();
    if (!arguments.contains(QStringLiteral("-prestart"))) {
        if (arguments.count() > 1 && (arguments.last() != QStringLiteral("-debugMode"))) {
            emit DeclarativeWebUtils::instance()->openUrlRequested(arguments.last());
        } else if (!DeclarativeWebUtils::instance()->firstUseDone()) {
            emit DeclarativeWebUtils::instance()->openUrlRequested("");
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
    emit DeclarativeWebUtils::instance()->openUrlRequested(url);
}

void Browser::openNewTabView()
{
    Q_D(Browser);
    d->closeEventFilter->cancelStopApplication();
    emit DeclarativeWebUtils::instance()->activateNewTabViewRequested();
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
