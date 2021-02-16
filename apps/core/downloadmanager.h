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

#include "downloadstatus.h"

#include <QObject>
#include <QHash>
#include <QString>
#include <QVariant>

class TransferEngineInterface;

class DownloadManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool pdfPrinting READ pdfPrinting NOTIFY pdfPrintingChanged FINAL)

    Q_ENUMS(DownloadStatus::Status)
public:
    static DownloadManager *instance();

    bool existActiveTransfers();

    bool pdfPrinting() const;

signals:
    void downloadStarted();
    void downloadStatusChanged(int downloadId, int status, QVariant info);
    void pdfPrintingChanged();
    void allTransfersCompleted();

public slots:
    void cancelActiveTransfers();
    void cancel(int downloadId);

private slots:
    void recvObserve(const QString message, const QVariant data);

private:
    explicit DownloadManager();
    ~DownloadManager();

    void checkAllTransfers();
    bool moveMyAppPackage(const QString &path) const;

    bool isMyApp(const QString &targetPath) const;
    bool needPlatformTransfersUpdate(const QString &targetPath) const;
    void finishMyAppDownload(const QString &targetPath) const;

    void cancelTransfer(int transferId);
    void restartTransfer(int transferId);

    void setPreferences();
    void setPdfPrinting(const bool pdfPrinting);

    // TODO: unlike Gecko downloads and Sailfish transfers these mappings
    //       are not persistent -> after user has browser closed transfers can't be
    //       restarted.
    QHash<qulonglong, int> m_download2transferMap;
    QHash<int, qulonglong> m_transfer2downloadMap;
    QHash<qulonglong, DownloadStatus::Status> m_statusCache;

    TransferEngineInterface *m_transferClient;

    bool m_pdfPrinting;

    friend class Browser;
};

#endif
