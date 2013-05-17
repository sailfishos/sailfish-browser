/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

#include "downloadmanager.h"
#include "qmozcontext.h"

DownloadManager::DownloadManager(BrowserService *service, QObject *parent)
    : QObject(parent),
      m_service(service)
{
    m_transferClient = new TransferEngineInterface("org.nemo.transferengine",
                                                   "/org/nemo/transferengine",
                                                   QDBusConnection::sessionBus(),
                                                   this);

    connect(QMozContext::GetInstance(), SIGNAL(recvObserve(const QString, const QVariant)),
            this, SLOT(recvObserve(const QString, const QVariant)));
    connect(service, SIGNAL(cancelTransferRequested(int)),
            this, SLOT(cancelTransfer(int)));
    connect(service, SIGNAL(restartTransferRequested(int)),
            this, SLOT(restartTransfer(int)));
}

void DownloadManager::recvObserve(const QString message, const QVariant data)
{
    if (message != "embed:download") {
        // here we are interested in download messages only
        return;
    }

    QVariantMap dataMap(data.toMap());
    QString msg(dataMap.value("msg").toString());
    qulonglong downloadId(dataMap.value("id").toULongLong());

    if (msg == "dl-start" && m_download2transferMap.contains(downloadId)) { // restart existing transfer
        m_transferClient->startTransfer(m_download2transferMap.value(downloadId));
        m_statusCache.insert(downloadId, DownloadStarted);
    } else if (msg == "dl-start") { // create new transfer
        QStringList callback;
        callback << "org.sailfishos.browser" << "/" << "org.sailfishos.browser";
        QDBusPendingReply<int> reply = m_transferClient->createDownload(dataMap.value("displayName").toString(),
                                                                        QString("image://theme/icon-launcher-browser"),
                                                                        QString("image://theme/icon-launcher-browser"),
                                                                        dataMap.value("sourceUrl").toString(),
                                                                        dataMap.value("mimeType").toString(),
                                                                        dataMap.value("size").toULongLong(),
                                                                        callback,
                                                                        QString("cancelTransfer"),
                                                                        QString("restartTransfer"));
        reply.waitForFinished();

        if (reply.isError()) {
            qWarning() << "DownloadManager::recvObserve: failed to get transfer ID!" << reply.error();
            return;
        }

        int transferId(reply.value());

        m_download2transferMap.insert(downloadId, transferId);
        m_transfer2downloadMap.insert(transferId, downloadId);
        m_statusCache.insert(downloadId, DownloadStarted);

        m_transferClient->startTransfer(transferId);
    } else if (msg == "dl-progress") {
        qreal progress(dataMap.value("percent").toULongLong() / 100.0);

        m_transferClient->updateTransferProgress(m_download2transferMap.value(downloadId),
                                                 progress);
    } else if (msg == "dl-done") {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferFinished,
                                         QString("success"));
        m_statusCache.insert(downloadId, DownloadDone);
    } else if (msg == "dl-fail") {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferInterrupted,
                                         QString("browser failure"));
        m_statusCache.insert(downloadId, DownloadFailed);
    } else if (msg == "dl-cancel") {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferCanceled,
                                         QString("download canceled"));
        m_statusCache.insert(downloadId, DownloadCanceled);
    }
}

void DownloadManager::cancelActiveTransfers()
{
    foreach (qulonglong downloadId, m_statusCache.keys()) {
        if (m_statusCache.value(downloadId) == DownloadStarted) {
            cancelTransfer(m_download2transferMap.value(downloadId));
        }
    }
}

void DownloadManager::cancelTransfer(int transferId)
{
    if (m_transfer2downloadMap.contains(transferId)) {
        QVariantMap data;
        data.insert("msg", "cancelDownload");
        data.insert("id", m_transfer2downloadMap.value(transferId));
        QMozContext::GetInstance()->sendObserve(QString("embedui:download"), QVariant(data));
    } else {
        m_transferClient->finishTransfer(transferId,
                                         TransferEngineData::TransferInterrupted,
                                         QString("Transfer got unavailable"));
    }
}

void DownloadManager::restartTransfer(int transferId)
{
    if (m_transfer2downloadMap.contains(transferId)) {
        QVariantMap data;
        data.insert("msg", "retryDownload");
        data.insert("id", m_transfer2downloadMap.value(transferId));
        QMozContext::GetInstance()->sendObserve(QString("embedui:download"), QVariant(data));
    } else {
        m_transferClient->finishTransfer(transferId,
                                         TransferEngineData::TransferInterrupted,
                                         QString("Transfer got unavailable"));
    }
}
