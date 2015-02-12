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
    connect(DBManager::instance(), SIGNAL(tabChanged(Tab)),
            this, SLOT(tabChanged(Tab)));
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
    m_tabs.clear();
    m_tabs = tabs;

    if (m_tabs.count() > 0) {
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
    } else {
        emit tabsCleared();
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

void PersistentTabModel::tabChanged(const Tab &tab)
{
#if DEBUG_LOGS
    qDebug() << "new tab data:" << &tab;
#endif
    if (m_tabs.isEmpty()) {
        qWarning() << "No tabs!";
        return;
    }

    int i = findTabIndex(tab.tabId());
    if (i > -1) {
        QVector<int> roles;
        Tab oldTab = m_tabs[i];
        if (oldTab.url() != tab.url()) {
            roles << UrlRole;
        }
        if (oldTab.title() != tab.title()) {
            roles << TitleRole;
        }
        if (oldTab.thumbnailPath() != tab.thumbnailPath()) {
            roles << ThumbPathRole;
        }
        m_tabs[i] = tab;
        QModelIndex start = index(i, 0);
        QModelIndex end = index(i, 0);
        emit dataChanged(start, end, roles);
    }

    if (tab.tabId() == m_activeTab.tabId()) {
        m_activeTab = tab;
        emit activeTabChanged(tab.tabId(), tab.tabId(), true);
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

void PersistentTabModel::updateTab(int tabId, QString url, QString title, QString path) {
    Q_UNUSED(title)
    Q_UNUSED(path)

    DBManager::instance()->updateTab(tabId, url, "", "");
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
