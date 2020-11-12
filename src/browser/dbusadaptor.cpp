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

#include "dbusadaptor.h"

DBusAdaptor::DBusAdaptor(BrowserService *browserService):
    QDBusAbstractAdaptor(browserService),
    m_BrowserService(browserService)
{
}

void DBusAdaptor::openUrl(const QStringList &args)
{
    m_BrowserService->openUrl(args);
}

void DBusAdaptor::activateNewTabView()
{
    m_BrowserService->activateNewTabView();
}

void DBusAdaptor::cancelTransfer(int transferId)
{
    m_BrowserService->cancelTransfer(transferId);
}

void DBusAdaptor::restartTransfer(int transferId)
{
    m_BrowserService->restartTransfer(transferId);
}

void DBusAdaptor::dumpMemoryInfo(const QString &fileName)
{
    m_BrowserService->dumpMemoryInfo(fileName);
}

void DBusAdaptor::closeBrowser()
{
    m_BrowserService->closeBrowser();
}

UIServiceDBusAdaptor::UIServiceDBusAdaptor(BrowserUIService *browserService)
    : QDBusAbstractAdaptor(browserService)
    , m_BrowserService(browserService)
{
}

void UIServiceDBusAdaptor::openUrl(const QStringList &args)
{
    m_BrowserService->openUrl(args);
}

void UIServiceDBusAdaptor::openSettings()
{
    m_BrowserService->openSettings();
}

void UIServiceDBusAdaptor::activateNewTabView()
{
    m_BrowserService->activateNewTabView();
}

void UIServiceDBusAdaptor::requestTab(int tabId, const QString &url)
{
    m_BrowserService->requestTab(tabId, url);
}

void UIServiceDBusAdaptor::closeTab(int tabId)
{
    m_BrowserService->closeTab(tabId);
}
