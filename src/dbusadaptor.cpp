/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/
#include "dbusadaptor.h"

DBusAdaptor::DBusAdaptor(BrowserService *browserService):
    QDBusAbstractAdaptor(browserService),
    m_BrowserService(browserService)
{
}

void DBusAdaptor::openUrl(QStringList args) {
    m_BrowserService->openUrl(args);
}

void DBusAdaptor::cancelTransfer(int transferId) {
    m_BrowserService->cancelTransfer(transferId);
}

void DBusAdaptor::restartTransfer(int transferId) {
    m_BrowserService->restartTransfer(transferId);
}
