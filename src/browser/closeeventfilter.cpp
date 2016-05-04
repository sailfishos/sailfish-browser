/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QCoreApplication>
#include "closeeventfilter.h"
#include "qmozcontext.h"
#include "declarativewebutils.h"
#include "dbmanager.h"

CloseEventFilter::CloseEventFilter(DownloadManager *dlMgr, QObject *parent)
    : QObject(parent),
      m_downloadManager(dlMgr)
{
    connect(QMozContext::GetInstance(), &QMozContext::lastWindowDestroyed,
            this, &CloseEventFilter::onLastWindowDestroyed);
    connect(QMozContext::GetInstance(), &QMozContext::destroyed,
            this, &CloseEventFilter::onContextDestroyed);
    connect(&m_shutdownWatchdog, &QTimer::timeout,
            this, &CloseEventFilter::onWatchdogTimeout);
}

void CloseEventFilter::stopApplication()
{
    MGConfItem closeAllTabsConf("/apps/sailfish-browser/settings/close_all_tabs");
    if (closeAllTabsConf.value(false).toBool()) {
        DBManager::instance()->removeAllTabs();
    }

    QMozContext::GetInstance()->stopEmbedding();
    // Give the engine 5 seconds to shut down. If it fails terminate
    // with a fatal error.
    m_shutdownWatchdog.start(5000);
}

void CloseEventFilter::onLastWindowDestroyed()
{
    if (m_downloadManager->existActiveTransfers()) {
        connect(m_downloadManager, SIGNAL(allTransfersCompleted()),
                this, SLOT(stopApplication()));
    } else {
        stopApplication();
    }
}

void CloseEventFilter::onContextDestroyed()
{
    qApp->exit();
}

void CloseEventFilter::onWatchdogTimeout()
{
    qFatal("Browser failed to terminate in acceptable time!");
}

void CloseEventFilter::cancelStopApplication()
{
    disconnect(m_downloadManager, SIGNAL(allTransfersCompleted()),
               this, SLOT(stopApplication()));
    m_shutdownWatchdog.stop();
}
