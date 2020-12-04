/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CLOSEEVENTFILTER_H
#define CLOSEEVENTFILTER_H

#include <QObject>
#include <QEvent>
#include <QTimer>
#include "downloadmanager.h"

class CloseEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit CloseEventFilter(DownloadManager *dlMgr, QObject *parent = 0);

public slots:
    void cancelStopApplication();

private slots:
    void stopApplication();
    void onLastWindowDestroyed();
    void onContextDestroyed();
    void onWatchdogTimeout();

private:
    DownloadManager *m_downloadManager;
    QTimer m_shutdownWatchdog;
};

#endif
