/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QVariant>

class TransferEngineInterface;

class DownloadManager : public QObject
{
    Q_OBJECT

public:
    static DownloadManager *instance();

    bool existActiveTransfers();

signals:
    void downloadStarted();
    void allTransfersCompleted();

public slots:
    void cancelActiveTransfers();

private slots:
    void recvObserve(const QString message, const QVariant data);
    void cancelTransfer(int transferId);
    void restartTransfer(int transferId);

private:
    explicit DownloadManager();
    ~DownloadManager();

    enum Status {
        DownloadStarted,
        DownloadDone,
        DownloadFailed,
        DownloadCanceled
    };

    void checkAllTransfers();
    QString aptoideApk(QString packageName);
    bool moveMyAppPackage(QString path);

    // TODO: unlike Gecko downloads and Sailfish transfers these mappings
    //       are not persistent -> after user has browser closed transfers can't be
    //       restarted.
    QHash<qulonglong, int> m_download2transferMap;
    QHash<int, qulonglong> m_transfer2downloadMap;
    QHash<qulonglong, Status> m_statusCache;

    TransferEngineInterface *m_transferClient;
};

#endif
