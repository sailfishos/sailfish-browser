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

CloseEventFilter::CloseEventFilter(DownloadManager *dlMgr, QObject *parent)
    : QObject(parent),
      m_downloadManager(dlMgr)
{
}

bool CloseEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        if (m_downloadManager->existActiveTransfers()) {
            connect(m_downloadManager, SIGNAL(allTransfersCompleted()),
                    this, SLOT(stopApplication()));
        } else {
            stopApplication();
        }
        return false;
    } else {
        return QObject::eventFilter(obj, event);
    }
}

void CloseEventFilter::stopApplication()
{
    emit DeclarativeWebUtils::instance()->beforeShutdown();
    QMozContext::GetInstance()->stopEmbedding();
    qApp->quit();
 }

void CloseEventFilter::cancelStopApplication()
{
    disconnect(m_downloadManager, SIGNAL(allTransfersCompleted()),
               this, SLOT(stopApplication()));
}
