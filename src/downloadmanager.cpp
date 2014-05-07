/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "downloadmanager.h"
#include "qmozcontext.h"
#include "declarativewebutils.h"

#include <transferengineinterface.h>
#include <transfertypes.h>
#include <QDir>
#include <QFile>
#include <QDebug>

#include <pwd.h>
#include <grp.h>
#include <unistd.h>

static DownloadManager *gSingleton = 0;

DownloadManager::DownloadManager()
    : QObject()
{
    m_transferClient = new TransferEngineInterface("org.nemo.transferengine",
                                                   "/org/nemo/transferengine",
                                                   QDBusConnection::sessionBus(),
                                                   this);
    connect(QMozContext::GetInstance(), SIGNAL(recvObserve(const QString, const QVariant)),
            this, SLOT(recvObserve(const QString, const QVariant)));
}

DownloadManager::~DownloadManager()
{
    gSingleton = 0;
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
        emit downloadStarted();
        QStringList callback;
        callback << "org.sailfishos.browser" << "/" << "org.sailfishos.browser";
        QDBusPendingReply<int> reply = m_transferClient->createDownload(dataMap.value("displayName").toString(),
                                                                        QString("image://theme/icon-launcher-browser"),
                                                                        QString("image://theme/icon-launcher-browser"),
                                                                        dataMap.value("targetPath").toString(),
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

        m_transferClient->startTransfer(transferId);
        m_statusCache.insert(downloadId, DownloadStarted);
    } else if (msg == "dl-progress") {
        qreal progress(dataMap.value("percent").toULongLong() / 100.0);

        m_transferClient->updateTransferProgress(m_download2transferMap.value(downloadId),
                                                 progress);
    } else if (msg == "dl-done") {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferFinished,
                                         QString("success"));
        m_statusCache.insert(downloadId, DownloadDone);
        checkAllTransfers();

        QString targetPath = dataMap.value("targetPath").toString();
        QFileInfo fileInfo(targetPath);
        if (fileInfo.completeSuffix() == QLatin1Literal("myapp")) {
            QString aptoideDownloadPath = QString("%1/.aptoide/").arg(DeclarativeWebUtils::instance()->downloadDir());
            QDir dir(aptoideDownloadPath);

            if (!dir.exists()) {
                if (!dir.mkpath(aptoideDownloadPath)) {
                    qWarning() << "Failed to create path for myapp download, aborting";
                    return;
                }
                uid_t uid = getuid();
                // assumes that correct groupname is same as username (e.g. nemo:nemo)
                int gid = getgrnam(getpwuid(uid)->pw_name)->gr_gid;
                chown(aptoideDownloadPath.toLatin1().data(), uid, gid);
                QFile::Permissions permissions(QFile::ExeOwner | QFile::ExeGroup | QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup);
                QFile::setPermissions(aptoideDownloadPath, permissions);
            }

            QFile file(targetPath);
            if (file.rename(aptoideDownloadPath + fileInfo.fileName())) {
                // Check if we have aptoide installed
                QString aptoideApk("/data/app/com.aptoide.partners.apk");
                if (QFile(aptoideApk).exists()) {
                    QProcess::execute("/usr/bin/apkd-launcher", QStringList() << aptoideApk << "com.aptoide.partners/com.aptoide.partners.AptoideJollaSupport");
                }
            }
        }
    } else if (msg == "dl-fail") {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferInterrupted,
                                         QString("browser failure"));
        m_statusCache.insert(downloadId, DownloadFailed);
        checkAllTransfers();
    } else if (msg == "dl-cancel") {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferCanceled,
                                         QString("download canceled"));
        m_statusCache.insert(downloadId, DownloadCanceled);
        checkAllTransfers();
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

DownloadManager *DownloadManager::instance()
{
    if (!gSingleton) {
        gSingleton = new DownloadManager();
    }
    return gSingleton;
}

bool DownloadManager::existActiveTransfers()
{
    bool exists(false);

    foreach (Status st, m_statusCache) {
        if (st == DownloadStarted) {
            exists = true;
            break;
        }
    }
    return exists;
}

void DownloadManager::checkAllTransfers()
{
    if (!existActiveTransfers()) {
        emit allTransfersCompleted();
    }
}
