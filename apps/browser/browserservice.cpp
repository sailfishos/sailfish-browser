/****************************************************************************
**
** Copyright (c) 2013 - 2019 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "browserservice.h"
#include "browserservice_p.h"
#include "declarativewebcontainer.h"
#include "browserapp.h"
#include "logging.h"

#include "dbusadaptor.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFileInfo>
#include <QCoreApplication>

namespace {
const auto ProcDir = QStringLiteral("/proc/%1");
const auto ErrorPidIsNotPrivileged = QStringLiteral("PID %1 is not in privileged group");
const auto ErrorPidIsNotOwnerService = QStringLiteral("PID %1 is not the owner of '%2'");
const auto ErrorPidIsNotTabOwner = QStringLiteral("PID %1 is not the owner of the tab");
const auto SailfishBrowserUiService = QStringLiteral("org.sailfishos.browser.ui");
const auto TransferEngine = QStringLiteral("org.nemo.transferengine");
}

#define GET_PID() connection().interface()->servicePid(message().service())

using DBusContext = std::pair<QDBusMessage, QDBusConnection>;

BrowserService::BrowserService(QObject * parent)
    : QObject(parent)
    , QDBusContext()
    , m_registered(true)
{
    new DBusAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    if(!connection.registerObject("/", this)
            || !connection.registerService(serviceName())) {
        m_registered = false;
    }
}

bool BrowserService::registered() const
{
    return m_registered;
}

QString BrowserService::serviceName() const
{
    return QStringLiteral("org.sailfishos.browser");
}

void BrowserService::openUrl(const QStringList &args)
{
    if(args.count() > 0) {
        emit openUrlRequested(args.first());
    } else {
        emit openUrlRequested(QString());
    }
}

void BrowserService::activateNewTabView()
{
    emit activateNewTabViewRequested();
}

void BrowserService::cancelTransfer(int transferId)
{
    if (!callerMatchesService(TransferEngine)) {
        return;
    }

    emit cancelTransferRequested(transferId);
}

void BrowserService::restartTransfer(int transferId)
{
    if (!callerMatchesService(TransferEngine)) {
        return;
    }

    emit restartTransferRequested(transferId);
}

void BrowserService::dumpMemoryInfo(const QString &fileName)
{
    // The privileged check will always fail when the browser is sandboxed
    // It can still be useful for debugging purposes running outside the sandbox
    if (!isPrivileged()) {
        return;
    }

    emit dumpMemoryInfoRequested(fileName);
}

bool BrowserService::isPrivileged() const
{
    bool isPrivileged = !calledFromDBus();
    if (!isPrivileged) {
        uint pid = GET_PID();
        QFileInfo info(ProcDir.arg(pid));
        isPrivileged = info.group() == "privileged" || info.owner() == "root";
    }
    if (!isPrivileged) {
        sendErrorReply(QDBusError::AccessDenied,
                ErrorPidIsNotPrivileged.arg(GET_PID()));
    }
    return isPrivileged;
}

bool BrowserService::callerMatchesService(const QString &serviceName) const
{
    uint callerServicePid = GET_PID().value();

    // Test this against pid of serviceName which works also inside
    // sandbox. If that matches, then the caller is serviceName.
    if (callerServicePid == connection().interface()->servicePid(serviceName).value()) {
        return true;
    }

    sendErrorReply(QDBusError::AccessDenied,
                   ErrorPidIsNotOwnerService.arg(callerServicePid).arg(serviceName));
    return false;
}

BrowserUIServicePrivate::BrowserUIServicePrivate()
    : registered(true)
{
}

BrowserUIService::BrowserUIService(QObject *parent)
    : QObject(parent)
    , QDBusContext()
    , d_ptr(new BrowserUIServicePrivate())
{
    Q_D(BrowserUIService);

    new UIServiceDBusAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    if(!connection.registerObject("/ui", this) ||
            !connection.registerService(SailfishBrowserUiService)) {
        d->registered = false;
    }
}

bool BrowserUIService::registered() const
{
    Q_D(const BrowserUIService);

    return d->registered;
}

QString BrowserUIService::serviceName() const
{
    return SailfishBrowserUiService;
}

void BrowserUIService::openUrl(const QStringList &args)
{
    if(args.count() > 0) {
        emit openUrlRequested(args.first());
    } else {
        emit openUrlRequested(QString());
    }
}

void BrowserUIService::openSettings()
{
    emit openSettingsRequested();
}

void BrowserUIService::activateNewTabView()
{
    emit activateNewTabViewRequested();
}

void BrowserUIService::requestTab(int tabId, const QString &url)
{
    DBusContext *context = new DBusContext(message(), connection());
    setDelayedReply(true);
    connect(DeclarativeWebContainer::instance(),
            &DeclarativeWebContainer::requestTabWithOwnerAsyncResult,
            this, &BrowserUIService::requestTabReturned, Qt::UniqueConnection);

    qCDebug(lcCoreLog) << "Received open tab request";
    DeclarativeWebContainer::instance()->requestTabWithOwnerAsync(tabId, url, getCallerPid(), static_cast<void*>(context));
}

void BrowserUIService::requestTabReturned(int tabId, void* context)
{
    DBusContext *dbusContext = static_cast<DBusContext*>(context);
    if (context != nullptr) {
        qCDebug(lcCoreLog) << "Replying to open tab request";
        QDBusMessage reply = dbusContext->first.createReply(tabId);
        dbusContext->second.send(reply);
        delete dbusContext;
    }
    emit showChrome();
}

void BrowserUIService::closeTab(int tabId)
{
    // Let the process that created the tab also destroy it
    if (!matchesOwner(DeclarativeWebContainer::instance()->tabOwner(tabId))) {
        sendErrorReply(QDBusError::AccessDenied,
                ErrorPidIsNotTabOwner.arg(GET_PID()));
        return;
    }

    DeclarativeWebContainer::instance()->closeTab(tabId);
    const QDBusMessage &msg = message();
    QDBusMessage reply = msg.createReply();
    connection().send(reply);
}

uint BrowserUIService::getCallerPid() const
{
    return calledFromDBus() ? GET_PID() : 0;
}

bool BrowserUIService::matchesOwner(uint pid) const
{
    uint caller = getCallerPid();
    return caller != 0 && caller == pid;
}
