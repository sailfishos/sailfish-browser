/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/
#include "browserservice.h"
#include "dbusadaptor.h"
#include <QDBusConnection>
#include <QDebug>

BrowserService::BrowserService(QObject * parent): QObject(parent)
{
    new DBusAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerService("org.sailfishos.browser");
    if(!connection.registerObject("/", this)) {
        qWarning() << "Cannot register to DBus, other instance likely running";
    }
}

void BrowserService::openUrl(QStringList args)
{
    if(args.count() > 0) {
        emit openUrlRequested(args.first());
    } else {
        emit openUrlRequested(QString());
    }
}

void BrowserService::cancelTransfer(int transferId)
{
    emit cancelTransferRequested(transferId);
}

void BrowserService::restartTransfer(int transferId)
{
    emit restartTransferRequested(transferId);
}
