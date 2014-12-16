/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativetabmodel.h"
#include "dbmanager.h"
#include "linkvalidator.h"
#include "declarativewebutils.h"

#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QUrl>

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

DeclarativeTabModel::DeclarativeTabModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_loaded(false)
    , m_waitingForNewTab(false)
    , m_nextTabId(DBManager::instance()->getMaxTabId() + 1)
{
    connect(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)),
            this, SLOT(tabsAvailable(QList<Tab>)));
    connect(DBManager::instance(), SIGNAL(tabChanged(Tab)),
            this, SLOT(tabChanged(Tab)));
    connect(DeclarativeWebUtils::instance(), SIGNAL(beforeShutdown()),
            this, SLOT(saveActiveTab()));
}

DeclarativeTabModel::~DeclarativeTabModel()
{
}

QHash<int, QByteArray> DeclarativeTabModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ThumbPathRole] = "thumbnailPath";
    roles[TitleRole] = "title";
    roles[UrlRole] = "url";
    roles[TabIdRole] = "tabId";
    return roles;
}

void DeclarativeTabModel::addTab(const QString& url, const QString &title) {
    if (!LinkValidator::navigable(QUrl(url))) {
        return;
    }
    int tabId = DBManager::instance()->createTab();
    int linkId = DBManager::instance()->createLink(tabId, url, title);

    Tab tab(tabId, Link(linkId, url, "", title), 0, 0);
#if DEBUG_LOGS
    qDebug() << "new tab data:" << &tab;
#endif
    int index = m_tabs.count();
    beginInsertRows(QModelIndex(), index, index);
    m_tabs.append(tab);
    endInsertRows();
    // We should trigger this only when
    // tab is added through new window request. In all other
    // case we should keep the new tab in background.
    updateActiveTab(tab, true);

    emit countChanged();
    emit tabAdded(tabId);

    m_nextTabId = ++tabId;
    emit nextTabIdChanged();

    setWaitingForNewTab(false);
}

int DeclarativeTabModel::nextTabId() const
{
    return m_nextTabId;
}

void DeclarativeTabModel::remove(int index) {
    if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
        bool removingActiveTab = activeTabIndex() == index;
        removeTab(m_tabs.at(index).tabId(), m_tabs.at(index).thumbnailPath(), index);
        if (removingActiveTab) {
            if (index >= m_tabs.count()) {
                --index;
            }

            activateTab(index, false);
        }
    }
}

void DeclarativeTabModel::removeTabById(int tabId, bool activeTab)
{
    int index = -1;
    if (!activeTab) {
        index = findTabIndex(tabId);
    }

    if (activeTab) {
        closeActiveTab();
    } else if (index >= 0){
        remove(index);
    }
}

void DeclarativeTabModel::clear()
{
    if (count() == 0)
        return;

    for (int i = m_tabs.count() - 1; i >= 0; --i) {
        removeTab(m_tabs.at(i).tabId(), m_tabs.at(i).thumbnailPath(), i);
    }

    setWaitingForNewTab(false);
}

bool DeclarativeTabModel::activateTab(const QString& url)
{
    for (int i = 0; i < m_tabs.size(); i++) {
        if (m_tabs.at(i).url() == url) {
            return activateTab(i);
        }
    }
    return false;
}

bool DeclarativeTabModel::activateTab(int index, bool loadActiveTab)
{
    if (index >= 0 && index < m_tabs.count()) {
        const Tab &newActiveTab = m_tabs.at(index);
#if DEBUG_LOGS
        qDebug() << "activate tab: " << index << &newActiveTab;
#endif
        updateActiveTab(newActiveTab, loadActiveTab);
        return true;
    } else {
        return false;
    }
}

bool DeclarativeTabModel::activateTabById(int tabId)
{
    int index = findTabIndex(tabId);
    if (index >= 0) {
        return activateTab(index);
    }
    return false;
}

/**
 * @brief DeclarativeTabModel::closeActiveTab
 * Closes the active tab and activates a tab next to the current tab. If possible
 * tab that is after the current tab is activated, then falling back to previous tabs, or
 * finally none (if all closed).
 */
void DeclarativeTabModel::closeActiveTab()
{
    if (!m_tabs.isEmpty()) {
        int index = activeTabIndex();
        removeTab(m_activeTab.tabId(), m_activeTab.thumbnailPath(), index);

        if (index >= m_tabs.count()) {
            --index;
        }

        activateTab(index, true);
    }
}

void DeclarativeTabModel::newTab(const QString &url, const QString &title, int parentId)
{
    setWaitingForNewTab(true);
    emit newTabRequested(url, title, parentId);
}

void DeclarativeTabModel::dumpTabs() const
{
    for (int i = 0; i < m_tabs.size(); i++) {
        qDebug() << "tab[" << i << "]:" << &m_tabs.at(i);
    }
}

int DeclarativeTabModel::activeTabIndex() const
{
    return findTabIndex(m_activeTab.tabId());
}

int DeclarativeTabModel::count() const
{
    return m_tabs.count();
}

int DeclarativeTabModel::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return m_tabs.count();
}

QVariant DeclarativeTabModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() > m_tabs.count())
        return QVariant();

    const Tab &tab = m_tabs.at(index.row());
    if (role == ThumbPathRole) {
        return tab.thumbnailPath();
    } else if (role == TitleRole) {
        return tab.title();
    } else if (role == UrlRole) {
        return tab.url();
    } else if (role == TabIdRole) {
        return tab.tabId();
    }
    return QVariant();
}

void DeclarativeTabModel::componentComplete()
{
}

bool DeclarativeTabModel::loaded() const
{
    return m_loaded;
}

void DeclarativeTabModel::setUnloaded()
{
    if (m_loaded) {
        m_loaded = false;
        emit loadedChanged();
    }
}

bool DeclarativeTabModel::waitingForNewTab() const
{
    return m_waitingForNewTab;
}

void DeclarativeTabModel::setWaitingForNewTab(bool waiting)
{
    if (m_waitingForNewTab != waiting) {
        m_waitingForNewTab = waiting;
        emit waitingForNewTabChanged();
    }
}

const QList<Tab> &DeclarativeTabModel::tabs() const
{
    return m_tabs;
}

const Tab &DeclarativeTabModel::activeTab() const
{
    return m_activeTab;
}

bool DeclarativeTabModel::contains(int tabId) const
{
    return findTabIndex(tabId) >= 0;
}

void DeclarativeTabModel::classBegin()
{
    DBManager::instance()->getAllTabs();
}

void DeclarativeTabModel::tabsAvailable(QList<Tab> tabs)
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

void DeclarativeTabModel::tabChanged(const Tab &tab)
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

void DeclarativeTabModel::updateUrl(int tabId, bool activeTab, QString url, bool backForwardNavigation, bool initialLoad)
{
    if (backForwardNavigation)
    {
        updateTabUrl(tabId, activeTab, url, false);
    } else {
        updateTabUrl(tabId, activeTab, url, !initialLoad);
    }
}

void DeclarativeTabModel::updateTitle(int tabId, bool activeTab, QString url, QString title)
{
    int tabIndex = findTabIndex(tabId);
    bool updateDb = false;
    int linkId = 0;
    if (tabIndex >= 0 && (m_tabs.at(tabIndex).title() != title || activeTab)) {
        QVector<int> roles;
        roles << TitleRole;
        m_tabs[tabIndex].setTitle(title);
        linkId = m_tabs.at(tabIndex).currentLink();
        emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
        updateDb = true;

        if (tabId == m_activeTab.tabId() && activeTab) {
            m_activeTab.setTitle(title);
        }
    }

    if (updateDb) {
        DBManager::instance()->updateTitle(tabId, linkId, url, title);
    }
}

void DeclarativeTabModel::removeTab(int tabId, const QString &thumbnail, int index)
{
#if DEBUG_LOGS
    qDebug() << "index:" << index << tabId;
#endif
    DBManager::instance()->removeTab(tabId);
    QFile f(thumbnail);
    if (f.exists()) {
        f.remove();
    }

    if (index >= 0) {
        if (activeTabIndex() == index) {
            m_activeTab.setTabId(0);
        }
        beginRemoveRows(QModelIndex(), index, index);
        m_tabs.removeAt(index);
        endRemoveRows();
    }

    emit countChanged();
    emit tabClosed(tabId);

    // This guarantees that view will reset its internal state
    // when the last tab got closed.
    if (m_tabs.isEmpty()) {
        emit tabsCleared();
    }
}

int DeclarativeTabModel::findTabIndex(int tabId) const
{
    for (int i = 0; i < m_tabs.size(); i++) {
        if (m_tabs.at(i).tabId() == tabId) {
            return i;
        }
    }
    return -1;
}

void DeclarativeTabModel::updateActiveTab(const Tab &activeTab, bool loadActiveTab)
{
#if DEBUG_LOGS
    qDebug() << "new active tab:" << &activeTab << "old active tab:" << &m_activeTab << "count:" << m_tabs.count();
#endif
    if (m_tabs.isEmpty()) {
        return;
    }

    if (m_activeTab != activeTab) {
        int oldTabId = m_activeTab.tabId();
        m_activeTab = activeTab;

        // If tab has changed, update active tab role.
        int tabIndex = activeTabIndex();
        if (oldTabId != m_activeTab.tabId() && tabIndex >= 0) {
            emit activeTabIndexChanged();
        }
        // To avoid blinking we don't expose "activeTabIndex" as a model role because
        // it should be updated over here and this is too early.
        // Instead, we pass current contentItem and activeTabIndex
        // when pushing the TabPage to the PageStack. This is the signal changes the
        // contentItem of WebView.
        emit activeTabChanged(oldTabId, activeTab.tabId(), loadActiveTab);
    }
}

void DeclarativeTabModel::updateTabUrl(int tabId, bool activeTab, const QString &url, bool navigate)
{
    if (!LinkValidator::navigable(QUrl(url))) {
#if DEBUG_LOGS
        qDebug() << "invalid url: " << url;
#endif
        return;
    }

    int tabIndex = findTabIndex(tabId);
    bool updateDb = false;
    if (tabIndex >= 0 && (m_tabs.at(tabIndex).url() != url || activeTab)) {
        QVector<int> roles;
        roles << UrlRole << TitleRole << ThumbPathRole;
        m_tabs[tabIndex].setUrl(url);

        if (navigate) {
            m_tabs[tabIndex].setNextLink(0);
            int currentLinkId = m_tabs.at(tabIndex).currentLink();
            m_tabs[tabIndex].setPreviousLink(currentLinkId);
            m_tabs[tabIndex].setCurrentLink(DBManager::instance()->nextLinkId());
        }
        m_tabs[tabIndex].setTitle("");
        m_tabs[tabIndex].setThumbnailPath("");

        if (tabId == m_activeTab.tabId()) {
            m_activeTab = m_tabs[tabIndex];
        }

        emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
        updateDb = true;
    }

    if (updateDb) {
        if (!navigate) {
            DBManager::instance()->updateTab(tabId, url, "", "");
        } else {
            DBManager::instance()->navigateTo(tabId, url, "", "");
        }
    }
}

void DeclarativeTabModel::updateThumbnailPath(int tabId, QString path)
{
    if (tabId <= 0)
        return;

    QVector<int> roles;
    roles << ThumbPathRole;
    for (int i = 0; i < m_tabs.count(); i++) {
        if (m_tabs.at(i).tabId() == tabId && m_tabs.at(i).thumbnailPath() != path) {
#if DEBUG_LOGS
            qDebug() << "model tab thumbnail updated: " << path << i << tabId;
#endif
            m_tabs[i].setThumbnailPath(path);
            QModelIndex start = index(i, 0);
            QModelIndex end = index(i, 0);
            emit dataChanged(start, end, roles);
            DBManager::instance()->updateThumbPath(tabId, path);
        }
    }
}

void DeclarativeTabModel::saveActiveTab() const
{
    DBManager::instance()->saveSetting("activeTabId", QString("%1").arg(m_activeTab.tabId()));
}
