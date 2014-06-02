/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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

void DBusAdaptor::dumpMemoryInfo(QString fileName)
{
    m_BrowserService->dumpMemoryInfo(fileName);
}
