/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
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

#define SAILFISH_BROWSER_SERVICE QLatin1String("org.sailfishos.browser")
#define SAILFISH_BROWSER_UI_SERVICE QLatin1String("org.sailfishos.browser.ui")
#define SAILFISH_BROWSER_CAPTIVE_PORTAL_SERVICE QLatin1String("org.sailfishos.browser.captiveportal")

#define IS_PRIVILEGED \
    if (!calledFromDBus()) { \
        return true; \
    } \
    uint pid = connection().interface()->servicePid(message().service()); \
    QFileInfo info(QString("/proc/%1").arg(pid)); \
    if (info.group() != "privileged" && info.owner() != "root") { \
        sendErrorReply(QDBusError::AccessDenied, \
            QString("PID %1 is not in privileged group").arg(pid)); \
        return false; \
    } \
    return true;

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
    if (BrowserApp::captivePortal())
        return SAILFISH_BROWSER_CAPTIVE_PORTAL_SERVICE;
    else
        return SAILFISH_BROWSER_SERVICE;
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
    if (!isPrivileged()) {
        return;
    }

    emit activateNewTabViewRequested();
}

void BrowserService::cancelTransfer(int transferId)
{
    if (!isPrivileged()) {
        return;
    }

    emit cancelTransferRequested(transferId);
}

void BrowserService::restartTransfer(int transferId)
{
    if (!isPrivileged()) {
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
    IS_PRIVILEGED;
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
            !connection.registerService(SAILFISH_BROWSER_UI_SERVICE)) {
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
    return SAILFISH_BROWSER_UI_SERVICE;
}

void BrowserUIService::openUrl(const QStringList &args)
{
    if(args.count() > 0) {
        emit openUrlRequested(args.first());
    } else {
        emit openUrlRequested(QString());
    }
}

void BrowserUIService::activateNewTabView()
{
    if (!isPrivileged()) {
        return;
    }

    emit activateNewTabViewRequested();
}

void BrowserUIService::requestTab(int tabId, const QString &url)
{
    if (!isPrivileged()) {
        return;
    }

    Q_D(BrowserUIService);

    int activatedTabId = DeclarativeWebContainer::instance()->activateTab(tabId, url);
    const QDBusMessage &msg = message();
    QDBusMessage reply = msg.createReply(activatedTabId);
    connection().send(reply);

    emit showChrome();
}

void BrowserUIService::closeTab(int tabId)
{
    if (!isPrivileged()) {
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
    IS_PRIVILEGED;
}
