/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativewebcontainer.h"
#include "privatetabmodel.h"
#include "dbmanager.h"

#define LINK_ID 1000

PrivateTabModel::PrivateTabModel(int nextTabId, DeclarativeWebContainer *webContainer)
    : DeclarativeTabModel(nextTabId, webContainer)
{
    // Startup should be synced to this.
    if (!m_loaded) {
        m_loaded = true;
        QMetaObject::invokeMethod(this, "loadedChanged", Qt::QueuedConnection);
    }
}

PrivateTabModel::~PrivateTabModel()
{
}

Tab PrivateTabModel::createTab(int tabId, QString url, QString title) {
    return Tab(tabId, Link(LINK_ID, url, "", title));
}

void PrivateTabModel::updateTitle(int tabId, QString url, QString title)
{
    Q_UNUSED(tabId)
    Q_UNUSED(url)
    Q_UNUSED(title)
}

void PrivateTabModel::removeTab(int tabId)
{
    Q_UNUSED(tabId)
}

void PrivateTabModel::navigateTo(int tabId, QString url, QString title, QString path) {
    Q_UNUSED(tabId)
    Q_UNUSED(url)
    Q_UNUSED(title)
    Q_UNUSED(path)
}

void PrivateTabModel::updateThumbPath(int tabId, QString path)
{
    Q_UNUSED(tabId)
    Q_UNUSED(path)
}
