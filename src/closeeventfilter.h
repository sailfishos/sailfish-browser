/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

#ifndef CLOSEEVENTFILTER_H
#define CLOSEEVENTFILTER_H

#include <QObject>
#include <QEvent>
#include "downloadmanager.h"
#include "browserservice.h"

class CloseEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit CloseEventFilter(DownloadManager *dlMgr, QObject *parent = 0);

public slots:
    void cancelStopApplication();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void stopApplication();

private:
    DownloadManager *m_downloadManager;
};

#endif
