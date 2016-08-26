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
#include "dbusadaptor.h"
#include <QDBusConnection>

#define SAILFISH_BROWSER_SERVICE QLatin1String("org.sailfishos.browser")
#define SAILFISH_BROWSER_UI_SERVICE QLatin1String("org.sailfishos.browser.ui")

BrowserService::BrowserService(QObject * parent)
    : QObject(parent)
    , m_registered(true)
{
    new DBusAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    if(!connection.registerObject("/", this)
            || !connection.registerService(SAILFISH_BROWSER_SERVICE)) {
        m_registered = false;
    }
}

bool BrowserService::registered() const
{
    return m_registered;
}

QString BrowserService::serviceName() const
{
    return SAILFISH_BROWSER_SERVICE;
}

void BrowserService::openUrl(QStringList args)
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
    emit cancelTransferRequested(transferId);
}

void BrowserService::restartTransfer(int transferId)
{
    emit restartTransferRequested(transferId);
}

void BrowserService::dumpMemoryInfo(QString fileName)
{
    emit dumpMemoryInfoRequested(fileName);
}


BrowserUIService::BrowserUIService(QObject * parent)
    : QObject(parent)
    , m_registered(true)
{
    new UIServiceDBusAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    if(!connection.registerService(SAILFISH_BROWSER_UI_SERVICE) ||
            !connection.registerObject("/ui", this)) {
        m_registered = false;
    }
}

bool BrowserUIService::registered() const
{
    return m_registered;
}

QString BrowserUIService::serviceName() const
{
    return SAILFISH_BROWSER_UI_SERVICE;
}

void BrowserUIService::openUrl(QStringList args)
{
    if(args.count() > 0) {
        emit openUrlRequested(args.first());
    } else {
        emit openUrlRequested(QString());
    }
}

void BrowserUIService::activateNewTabView()
{
    emit activateNewTabViewRequested();
}
