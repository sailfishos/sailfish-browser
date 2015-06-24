/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "persistenttabmodel.h"
#include "dbmanager.h"
#include "declarativewebutils.h"

PersistentTabModel::PersistentTabModel(QObject *parent)
    : DeclarativeTabModel(DBManager::instance()->getMaxTabId() + 1, parent)
{
    connect(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)),
            this, SLOT(tabsAvailable(QList<Tab>)));
    connect(DeclarativeWebUtils::instance(), SIGNAL(beforeShutdown()),
            this, SLOT(saveActiveTab()));

    DBManager::instance()->getAllTabs();
}

PersistentTabModel::~PersistentTabModel()
{
}

void PersistentTabModel::tabsAvailable(QList<Tab> tabs)
{
    beginResetModel();
    int oldCount = count();

    // Clear always previous tabs
    clear();

    if (tabs.count() > 0) {
        m_tabs = tabs;
        QString activeTabId = DBManager::instance()->getSetting("activeTabId");
        bool ok = false;
        int tabId = activeTabId.toInt(&ok);
        int index = findTabIndex(tabId);
        if (index >= 0) {
            m_activeTab = m_tabs[index];
        } else {
            // Fallback for browser update as this "activeTabId" is a new setting.
            m_activeTab = m_tabs[0];
        }
        emit activeTabIndexChanged();
    }

    endResetModel();

    if (count() != oldCount) {
        emit countChanged();
    }

    int maxTabId = DBManager::instance()->getMaxTabId();
    if (m_nextTabId != maxTabId + 1) {
        m_nextTabId = maxTabId + 1;
        emit nextTabIdChanged();
    }

    // Startup should be synced to this.
    if (!m_loaded) {
        m_loaded = true;
        emit loadedChanged();
    }
}

int PersistentTabModel::createTab() {
    return DBManager::instance()->createTab();
}

int PersistentTabModel::createLink(int tabId, QString url, QString title) {
    return DBManager::instance()->createLink(tabId, url, title);
}

void PersistentTabModel::updateTitle(int tabId, int linkId, QString url, QString title)
{
    DBManager::instance()->updateTitle(tabId, linkId, url, title);
}

void PersistentTabModel::removeTab(int tabId)
{
    DBManager::instance()->removeTab(tabId);
}

int PersistentTabModel::nextLinkId() {
    return DBManager::instance()->nextLinkId();
}

void PersistentTabModel::navigateTo(int tabId, QString url, QString title, QString path) {
    Q_UNUSED(title)
    Q_UNUSED(path)

    DBManager::instance()->navigateTo(tabId, url, "", "");
}

void PersistentTabModel::updateThumbPath(int tabId, QString path)
{
    DBManager::instance()->updateThumbPath(tabId, path);
}

void PersistentTabModel::saveActiveTab() const
{
    DBManager::instance()->saveSetting("activeTabId", QString("%1").arg(m_activeTab.tabId()));
}
