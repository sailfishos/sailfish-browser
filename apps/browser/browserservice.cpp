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

#include "dbusadaptor.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFileInfo>
#include <QCoreApplication>

namespace {
const auto ProcDir = QStringLiteral("/proc/%1");
const auto ErrorPidIsNotPrivileged = QStringLiteral("PID %1 is not in privileged group");
const auto ErrorPidIsNotOwnerServiceOrPrivileged = QStringLiteral("PID %1 is not the owner of '%2' or in privileged group");
const auto SailfishBrowserUiService = QStringLiteral("org.sailfishos.browser.ui");
const auto TransferEngine = QStringLiteral("org.nemo.transferengine");
}

#define GET_PID() connection().interface()->servicePid(message().service())

#define IS_PRIVILEGED() \
    if (!calledFromDBus()) { \
        return true; \
    } \
    uint pid = GET_PID(); \
    QFileInfo info(ProcDir.arg(pid)); \
    return info.group() == "privileged" || info.owner() == "root";

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
    if (!isPrivileged()) {
        return;
    }

    emit dumpMemoryInfoRequested(fileName);
}

bool BrowserService::isPrivileged() const
{
    auto isPrivileged = [=] {
        IS_PRIVILEGED();
    };
    if (!isPrivileged()) {
        sendErrorReply(QDBusError::AccessDenied,
                ErrorPidIsNotPrivileged.arg(GET_PID()));
        return false;
    }
    return true;
}

bool BrowserService::callerMatchesService(const QString &serviceName) const
{
    auto isPrivileged = [=] {
        IS_PRIVILEGED();
    };

    uint callerServicePid = GET_PID().value();

    // Test this against pid of serviceName which works also inside
    // sandbox. If that matches, then the caller is serviceName.
    if (isPrivileged() || callerServicePid == connection().interface()->servicePid(serviceName).value()) {
        return true;
    }

    sendErrorReply(QDBusError::AccessDenied,
                   ErrorPidIsNotOwnerServiceOrPrivileged.arg(callerServicePid).arg(serviceName));
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
    Q_D(BrowserUIService);

    int activatedTabId = DeclarativeWebContainer::instance()->requestTabWithOwner(tabId, url, getCallerPid());
    const QDBusMessage &msg = message();
    QDBusMessage reply = msg.createReply(activatedTabId);
    connection().send(reply);

    emit showChrome();
}

void BrowserUIService::closeTab(int tabId)
{
    // Let privileged and the process that created the tab to also destroy it
    if (!isPrivileged() && !matchesOwner(DeclarativeWebContainer::instance()->tabOwner(tabId))) {
        sendErrorReply(QDBusError::AccessDenied,
                QStringLiteral("PID %1 is not the owner or in privileged group").arg(GET_PID()));
        return;
    }

    Q_D(BrowserUIService);

    DeclarativeWebContainer::instance()->closeTab(tabId);
    const QDBusMessage &msg = message();
    QDBusMessage reply = msg.createReply();
    connection().send(reply);
}

bool BrowserUIService::isPrivileged() const
{
    IS_PRIVILEGED();
}

bool BrowserUIService::callerMatchesService(const QString &serviceName) const
{
    uint callerServicePid = GET_PID().value();

    // Test this against pid of serviceName which works also inside
    // sandbox. If that matches, then the caller is serviceName.
    if (isPrivileged() || callerServicePid == connection().interface()->servicePid(serviceName).value()) {
        return true;
    }

    sendErrorReply(QDBusError::AccessDenied,
                   ErrorPidIsNotOwnerServiceOrPrivileged.arg(callerServicePid).arg(serviceName));
    return false;
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
