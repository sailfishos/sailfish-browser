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
#include "downloadmimetypehandler.h"
#include "qmozcontext.h"
#include "browserpaths.h"

#include <transferengineinterface.h>
#include <transfertypes.h>
#include <QDir>
#include <QFile>
#include <QDebug>

#if defined(arm) \
    || defined(__arm__) \
    || defined(ARM) \
    || defined(_ARM_)
#define CPU_ARM 1
#endif

static DownloadManager *gSingleton = 0;

DownloadManager::DownloadManager()
    : QObject()
    , m_initialized(false)
{
    m_transferClient = new TransferEngineInterface("org.nemo.transferengine",
                                                   "/org/nemo/transferengine",
                                                   QDBusConnection::sessionBus(),
                                                   this);
    connect(QMozContext::GetInstance(), SIGNAL(onInitialized()),
            this, SLOT(setPreferences()));
    connect(QMozContext::GetInstance(), SIGNAL(recvObserve(const QString, const QVariant)),
            this, SLOT(recvObserve(const QString, const QVariant)));
}

DownloadManager::~DownloadManager()
{
    gSingleton = 0;
}

void DownloadManager::recvObserve(const QString message, const QVariant data)
{

    if (message == "download-manager-initialized" && !m_initialized) {
        m_initialized = true;
        emit initializedChanged();
    } else if (message != "embed:download") {
        // here we are interested in download messages only
        return;
    }

    QVariantMap dataMap(data.toMap());
    QString msg(dataMap.value("msg").toString());
    qulonglong downloadId(dataMap.value("id").toULongLong());

    if (msg == QLatin1Literal("dl-start") && m_download2transferMap.contains(downloadId)) { // restart existing transfer
        m_transferClient->startTransfer(m_download2transferMap.value(downloadId));
        m_statusCache.insert(downloadId, DownloadStarted);
    } else if (msg == QLatin1Literal("dl-start")) { // create new transfer
        emit downloadStarted();
        QStringList callback;
        QLatin1Literal browserInterface("org.sailfishos.browser");
        callback << browserInterface << QLatin1Literal("/") << browserInterface;
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
    } else if (msg == QLatin1Literal("dl-progress")) {
        qreal progress(dataMap.value(QStringLiteral("percent")).toULongLong() / 100.0);

        m_transferClient->updateTransferProgress(m_download2transferMap.value(downloadId),
                                                 progress);
    } else if (msg == QLatin1Literal("dl-done")) {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferFinished,
                                         QString("success"));
        m_statusCache.insert(downloadId, DownloadDone);
        checkAllTransfers();

        QString targetPath = dataMap.value(QStringLiteral("targetPath")).toString();
        QFileInfo fileInfo(targetPath);
        if (fileInfo.completeSuffix() == QLatin1Literal("myapp")) {
            QString rootNameSpace("com.aptoide.partners%1");
            QString aptoideSupport = rootNameSpace.arg(QLatin1Literal(".AptoideJollaSupport"));
#if CPU_ARM
            QString packageName = rootNameSpace.arg(QString());
#else
            QString packageName = rootNameSpace.arg(QLatin1Literal(".jolla_tablet_store"));
#endif
            QString apkName = QString("%1.apk").arg(packageName);
            // TODO: Add proper checking that Aptoide is installed (JB#33047)
            if (moveMyAppPackage(targetPath)) {
                QProcess::execute(QStringLiteral("/usr/bin/apkd-launcher"), QStringList() << apkName << QString("%1/%2").arg(packageName).arg(aptoideSupport));
            }
        }
    } else if (msg == QLatin1Literal("dl-fail")) {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferInterrupted,
                                         QString("browser failure"));
        m_statusCache.insert(downloadId, DownloadFailed);
        checkAllTransfers();
    } else if (msg == QLatin1Literal("dl-cancel")) {
        m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                         TransferEngineData::TransferCanceled,
                                         QString("download canceled"));
        m_statusCache.insert(downloadId, DownloadCanceled);
        checkAllTransfers();
    }
}

bool DownloadManager::moveMyAppPackage(QString path)
{
    QString aptoideDownloadPath = QString("%1/.aptoide/").arg(BrowserPaths::downloadLocation());
    if (!BrowserPaths::createDirectory(aptoideDownloadPath)) {
        qWarning() << "Failed to create path for myapp download, aborting";
        return false;
    }

    QFile file(path);
    QFileInfo fileInfo(file);
    QString newPath(aptoideDownloadPath + fileInfo.fileName());
    QFile obsoleteFile(newPath);
    if (obsoleteFile.exists() && !obsoleteFile.remove()) {
        qWarning() << "Failed to remove obsolete myapp file, aborting";
        return false;
    }

    if (!file.rename(newPath)) {
        qWarning() << "Failed to move myapp file to aptoide's folder, aborting";
        // Avoid generating ~/Downloads/<name>(2).myapp file in case user downloads the same file again
        file.remove();
        return false;
    }

    return true;
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

void DownloadManager::setPreferences()
{
    QMozContext* mozContext = QMozContext::GetInstance();
    Q_ASSERT(mozContext->initialized());

    // Use autodownload, never ask
    mozContext->setPref(QString("browser.download.useDownloadDir"), QVariant(true));
    // see https://developer.mozilla.org/en-US/docs/Download_Manager_preferences
    // Use custom downloads location defined in browser.download.dir
    mozContext->setPref(QString("browser.download.folderList"), QVariant(2));
    mozContext->setPref(QString("browser.download.dir"), BrowserPaths::downloadLocation());
    // Downloads should never be removed automatically
    mozContext->setPref(QString("browser.download.manager.retention"), QVariant(2));
    // Downloads will be canceled on quit
    // TODO: this doesn't really work. Instead the incomplete downloads get restarted
    //       on browser launch.
    mozContext->setPref(QString("browser.download.manager.quitBehavior"), QVariant(2));
    // TODO: this doesn't really work too
    mozContext->setPref(QString("browser.helperApps.deleteTempFileOnExit"), QVariant(true));

    DownloadMimetypeHandler::update();
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

bool DownloadManager::initialized()
{
    return m_initialized;
}

void DownloadManager::checkAllTransfers()
{
    if (!existActiveTransfers()) {
        emit allTransfersCompleted();
    }
}
