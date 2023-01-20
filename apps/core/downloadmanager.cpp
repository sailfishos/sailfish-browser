/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "downloadmanager.h"
#include "downloadmimetypehandler.h"
#include "browserpaths.h"
#include "logging.h"

#include <transferengineinterface.h>
#include <transfertypes.h>
#include <QDir>
#include <QFile>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDebug>

#include <webengine.h>
#include <webenginesettings.h>

#if defined(arm) \
    || defined(__arm__) \
    || defined(ARM) \
    || defined(_ARM_)
#define CPU_ARM 1
#endif

static DownloadManager *gSingleton = 0;

DownloadManager::DownloadManager()
    : QObject(),
      m_transferClient(nullptr),
      m_pdfPrinting(false)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
            "org.nemo.transferengine.discovery",
            "/",
            "org.nemo.transferengine.discovery",
            "peerToPeerAddress");
    QDBusConnection::sessionBus().callWithCallback(
            msg,
            this,
            SLOT(discoverySucceeded(QString)),
            SLOT(discoveryFailed()),
            15000); // allow 15 seconds for the transfer engine to start.

    setPreferences();
    SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();
    connect(webEngine, &SailfishOS::WebEngine::recvObserve,
            this, &DownloadManager::recvObserve);

    // Ignore the download info argument of the downloadStatusChanged signal.
    connect(this, &DownloadManager::downloadStatusChanged, [=](int downloadId, int status) {
        m_statusCache.insert(downloadId, static_cast<DownloadStatus::Status>(status));
    });
}

DownloadManager::~DownloadManager()
{
    gSingleton = 0;
}

void DownloadManager::discoveryFailed()
{
    qWarning() << "Unable to discover transfer-engine IPC socket address";
}

void DownloadManager::discoverySucceeded(const QString &p2pAddress)
{
    static int connectionCount = 0;
    const QString name = QString::fromLatin1("transfer-engine-connection-dm-%1").arg(connectionCount++);
    QDBusConnection p2pc = QDBusConnection::connectToPeer(p2pAddress, name);
    if (!p2pc.isConnected()) {
        qWarning() << "Unable to connect to transfer-engine on address:"
                   << p2pAddress << ":" << p2pc.lastError()
                   << p2pc.lastError().type() << p2pc.lastError().name();
    } else {
        m_transferClient = new TransferEngineInterface(
                "org.nemo.transferengine",
                "/org/nemo/transferengine",
                p2pc,
                this);
    }
}

void DownloadManager::recvObserve(const QString message, const QVariant data)
{
    if (message != "embed:download") {
        // here we are interested in download messages only
        return;
    }

    if (!m_transferClient) {
        qWarning() << "DownloadManager::recvObserve: transfer client not initialized!";
    }

    QVariantMap dataMap(data.toMap());
    QString msg = dataMap.value(QStringLiteral("msg")).toString();
    QString targetPath = dataMap.value(QStringLiteral("targetPath")).toString();
    bool isSaveAsPdf = dataMap.value(QStringLiteral("saveAsPdf")).toBool();
    qulonglong downloadId(dataMap.value(QStringLiteral("id")).toULongLong());
    bool needPlatformTransfersUpdate = this->needPlatformTransfersUpdate(targetPath);

    qCInfo(lcDownloadLog) << "Browser received embed:download message:" << msg
                          << "needs platform transfer:" << needPlatformTransfersUpdate
                          << "target path:" << targetPath
                          << "existing transfer:" << m_download2transferMap.contains(downloadId);

    if (isSaveAsPdf) {
        if (msg == QLatin1Literal("dl-start")) {
            setPdfPrinting(true);
        } else if (msg == QLatin1Literal("dl-done") ||
                  msg == QLatin1Literal("dl-fail") ||
                  msg == QLatin1Literal("dl-cancel")) {
            setPdfPrinting(false);
        }
    }

    if (msg == QLatin1Literal("dl-start")
            && needPlatformTransfersUpdate
            && m_download2transferMap.contains(downloadId)) { // restart existing transfer
        m_transferClient->startTransfer(m_download2transferMap.value(downloadId));
        emit downloadStatusChanged(downloadId, DownloadStatus::Started, data);
    } else if (msg == QLatin1Literal("dl-start")) { // create new transfer
        emit downloadStarted();
        if (needPlatformTransfersUpdate) {
            QStringList callback;
            QLatin1Literal browserInterface("org.sailfishos.browser");
            callback << browserInterface << QLatin1Literal("/") << browserInterface;
            QDBusPendingReply<int> reply = m_transferClient->createDownload(dataMap.value("displayName").toString(),
                                                                            QString("image://theme/icon-launcher-browser"),
                                                                            QString("image://theme/icon-launcher-browser"),
                                                                            targetPath,
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
            emit downloadStatusChanged(downloadId, DownloadStatus::Started, data);
        }
    } else if (msg == QLatin1Literal("dl-progress") && needPlatformTransfersUpdate) {
        qreal progress(dataMap.value(QStringLiteral("percent")).toULongLong() / 100.0);
        qCInfo(lcDownloadLog) << "Browser update download progress:" << progress;
        m_transferClient->updateTransferProgress(m_download2transferMap.value(downloadId), progress);
    } else if (msg == QLatin1Literal("dl-done")) {
        qCInfo(lcDownloadLog) << "Browser download done.";
        if (needPlatformTransfersUpdate) {
            m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                             TransferEngineData::TransferFinished,
                                             QString("success"));
            emit downloadStatusChanged(downloadId, DownloadStatus::Done, data);
        } else if (isMyApp(targetPath)) {
            finishMyAppDownload(targetPath);
        }
        checkAllTransfers();
    } else if (msg == QLatin1Literal("dl-fail")) {
        if (needPlatformTransfersUpdate) {
            m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                             TransferEngineData::TransferInterrupted,
                                             QString("browser failure"));
            emit downloadStatusChanged(downloadId, DownloadStatus::Failed, data);
        }
        checkAllTransfers();
    } else if (msg == QLatin1Literal("dl-cancel")) {
        if (needPlatformTransfersUpdate) {
            m_transferClient->finishTransfer(m_download2transferMap.value(downloadId),
                                             TransferEngineData::TransferCanceled,
                                             QString("download canceled"));
            emit downloadStatusChanged(downloadId, DownloadStatus::Canceled, data);
        }
        checkAllTransfers();
    }
}

bool DownloadManager::moveMyAppPackage(const QString &path) const
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

bool DownloadManager::isMyApp(const QString &targetPath) const
{
    return QFileInfo(targetPath).completeSuffix() == QLatin1Literal("myapp");
}

bool DownloadManager::needPlatformTransfersUpdate(const QString &targetPath) const
{
    return !isMyApp(targetPath);
}

void DownloadManager::finishMyAppDownload(const QString &targetPath) const
{
    Q_ASSERT(isMyApp(targetPath));

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

void DownloadManager::cancelActiveTransfers()
{
    for (qulonglong downloadId : m_statusCache.keys()) {
        if (m_statusCache.value(downloadId) == DownloadStatus::Started) {
            cancelTransfer(m_download2transferMap.value(downloadId));
        }
    }
}

void DownloadManager::cancel(int downloadId)
{
    QVariantMap data;
    data.insert("msg", "cancelDownload");
    data.insert("id", downloadId);
    SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();
    webEngine->notifyObservers(QString("embedui:download"), QVariant(data));
}

void DownloadManager::cancelTransfer(int transferId)
{
    if (m_transfer2downloadMap.contains(transferId)) {
        cancel(m_transfer2downloadMap.value(transferId));
    } else if (m_transferClient) {
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
        SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();
        webEngine->notifyObservers(QString("embedui:download"), QVariant(data));
    } else if (m_transferClient) {
        m_transferClient->finishTransfer(transferId,
                                         TransferEngineData::TransferInterrupted,
                                         QString("Transfer got unavailable"));
    }
}

void DownloadManager::setPreferences()
{
    SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();
    webEngineSettings->setPreference(QString("browser.download.useJSTransfer"), QVariant(true));
    // see https://developer.mozilla.org/en-US/docs/Download_Manager_preferences

    // NS_PREF_DOWNLOAD_FOLDERLIST of nsExternalHelperAppService
    webEngineSettings->setPreference(QString("browser.download.folderList"), QVariant(2));

    // Partial downloads are removed in the embedlite-components (see JB#50127)
    DownloadMimetypeHandler::update();
}

void DownloadManager::setPdfPrinting(const bool pdfPrinting) {
    if (m_pdfPrinting != pdfPrinting) {
        m_pdfPrinting = pdfPrinting;
        emit pdfPrintingChanged();
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

    for (DownloadStatus::Status status : m_statusCache) {
        if (status == DownloadStatus::Started) {
            exists = true;
            break;
        }
    }
    return exists;
}

bool DownloadManager::pdfPrinting() const
{
    return m_pdfPrinting;
}

void DownloadManager::checkAllTransfers()
{
    if (!existActiveTransfers()) {
        emit allTransfersCompleted();
    }
}
