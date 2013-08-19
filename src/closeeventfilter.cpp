/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

#include <QCoreApplication>
#include "closeeventfilter.h"
#include "qmozcontext.h"

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
    QMozContext::GetInstance()->stopEmbedding();
    qApp->quit();
 }

void CloseEventFilter::cancelStopApplication()
{
    disconnect(m_downloadManager, SIGNAL(allTransfersCompleted()),
               this, SLOT(stopApplication()));
}
