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

#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QUrl>

#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "declarativetabmodel.h"
#include "linkvalidator.h"

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

DeclarativeTabModel::DeclarativeTabModel(int nextTabId, DeclarativeWebContainer *webContainer)
    : QAbstractListModel(webContainer)
    , m_activeTabId(0)
    , m_loaded(false)
    , m_waitingForNewTab(false)
    , m_nextTabId(nextTabId)
    , m_webContainer(webContainer)
{
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
    roles[ActiveRole] = "activeTab";
    roles[TabIdRole] = "tabId";
    return roles;
}

void DeclarativeTabModel::addTab(const QString& url, const QString &title, int index) {
    if (!LinkValidator::navigable(QUrl(url))) {
        return;
    }

    Q_ASSERT(index >= 0 && index <= m_tabs.count());

    Tab tab = createTab(m_nextTabId, url, title);

#if DEBUG_LOGS
    qDebug() << "new tab data:" << &tab;
#endif
    beginInsertRows(QModelIndex(), index, index);
    m_tabs.insert(index, tab);
    endInsertRows();
    // We should trigger this only when
    // tab is added through new window request. In all other
    // case we should keep the new tab in background.
    updateActiveTab(tab, true);

    emit countChanged();
    emit tabAdded(tab.tabId());

    m_nextTabId = tab.tabId() + 1;
}

int DeclarativeTabModel::nextTabId() const
{
    return m_nextTabId;
}

void DeclarativeTabModel::remove(int index) {
    if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
        bool removingActiveTab = activeTabIndex() == index;
        int newActiveIndex = 0;
        if (removingActiveTab) {
            newActiveIndex = nextActiveTabIndex(index);
        }

        removeTab(m_tabs.at(index).tabId(), m_tabs.at(index).thumbnailPath(), index);
        if (removingActiveTab) {
            activateTab(newActiveIndex, false);
        }
    }
}

void DeclarativeTabModel::removeTabById(int tabId, bool activeTab)
{
    if (activeTab) {
        closeActiveTab();
    } else {
        int index = findTabIndex(tabId);
        if (index >= 0) {
            remove(index);
        }
    }
}

void DeclarativeTabModel::clear()
{
    if (count() == 0)
        return;

    for (int i = m_tabs.count() - 1; i >= 0; --i) {
        removeTab(m_tabs.at(i).tabId(), m_tabs.at(i).thumbnailPath(), i);
    }

    setWaitingForNewTab(true);
}

bool DeclarativeTabModel::activateTab(const QString& url)
{
    for (int i = 0; i < m_tabs.size(); i++) {
        if (m_tabs.at(i).url() == url) {
            activateTab(i);
            return true;
        }
    }
    return false;
}

void DeclarativeTabModel::activateTab(int index, bool loadActiveTab)
{
    if (m_tabs.isEmpty()) {
        return;
    }

    index = qBound<int>(0, index, m_tabs.count() - 1);
    const Tab &newActiveTab = m_tabs.at(index);
#if DEBUG_LOGS
    qDebug() << "activate tab: " << index << &newActiveTab;
#endif
    updateActiveTab(newActiveTab, loadActiveTab);
}

bool DeclarativeTabModel::activateTabById(int tabId)
{
    int index = findTabIndex(tabId);
    if (index >= 0) {
        activateTab(index);
        return true;
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
        int newActiveIndex = nextActiveTabIndex(index);
        removeTab(m_activeTabId, m_tabs.at(index).thumbnailPath(), index);
        activateTab(newActiveIndex, true);
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
    return findTabIndex(m_activeTabId);
}

int DeclarativeTabModel::activeTabId() const
{
    return m_activeTabId;
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
    } else if (role == ActiveRole) {
        return tab.tabId() == m_activeTabId;
    } else if (role == TabIdRole) {
        return tab.tabId();
    }
    return QVariant();
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
    Q_ASSERT(contains(m_activeTabId));
    return m_tabs.at(findTabIndex(m_activeTabId));
}

bool DeclarativeTabModel::contains(int tabId) const
{
    return findTabIndex(tabId) >= 0;
}

void DeclarativeTabModel::updateUrl(int tabId, const QString &url, bool initialLoad)
{
    if (!LinkValidator::navigable(QUrl(url))) {
#if DEBUG_LOGS
        qDebug() << "invalid url: " << url;
#endif
        return;
    }

    int tabIndex = findTabIndex(tabId);
    bool isActiveTab = m_activeTabId == tabId;
    bool updateDb = false;
    if (tabIndex >= 0 && (m_tabs.at(tabIndex).url() != url || isActiveTab)) {
        QVector<int> roles;
        roles << UrlRole << ThumbPathRole;
        m_tabs[tabIndex].setUrl(url);

        if (!initialLoad) {
            m_tabs[tabIndex].setCurrentLink(nextLinkId());
            updateDb = true;
        }
        m_tabs[tabIndex].setThumbnailPath("");

        emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
    }

    if (updateDb) {
        navigateTo(tabId, url, "", "");
    }
}

void DeclarativeTabModel::removeTab(int tabId, const QString &thumbnail, int index)
{
#if DEBUG_LOGS
    qDebug() << "index:" << index << tabId;
#endif
    removeTab(tabId);
    QFile f(thumbnail);
    if (f.exists()) {
        f.remove();
    }

    if (index >= 0) {
        if (activeTabIndex() == index) {
            m_activeTabId = 0;
        }
        beginRemoveRows(QModelIndex(), index, index);
        m_tabs.removeAt(index);
        endRemoveRows();
    }

    emit countChanged();
    emit tabClosed(tabId);
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
    qDebug() << "new active tab:" << &activeTab << "old active tab:" << m_activeTabId << "count:" << m_tabs.count();
#endif
    if (m_tabs.isEmpty()) {
        return;
    }

    if (m_activeTabId != activeTab.tabId()) {
        int oldTabId = m_activeTabId;
        m_activeTabId = activeTab.tabId();

        setWaitingForNewTab(true);

        // If tab has changed, update active tab role.
        int tabIndex = activeTabIndex();
        if (tabIndex >= 0) {
            QVector<int> roles;
            roles << ActiveRole;
            int oldIndex = findTabIndex(oldTabId);
            if (oldIndex >= 0) {
                emit dataChanged(index(oldIndex), index(oldIndex), roles);
            }
            emit dataChanged(index(tabIndex), index(tabIndex), roles);
            emit activeTabIndexChanged();
        }
        // To avoid blinking we don't expose "activeTabIndex" as a model role because
        // it should be updated over here and this is too early.
        // Instead, we pass current contentItem and activeTabIndex
        // when pushing the TabPage to the PageStack. This is the signal changes the
        // contentItem of WebView.
        emit activeTabChanged(activeTab.tabId(), loadActiveTab);
    }
}

void DeclarativeTabModel::setWebContainer(DeclarativeWebContainer *webContainer)
{
    m_webContainer = webContainer;
}

int DeclarativeTabModel::nextActiveTabIndex(int index)
{
    if (m_webContainer && m_webContainer->webPage() && m_webContainer->webPage()->parentId() > 0) {
        int newActiveTabId = m_webContainer->findParentTabId(m_webContainer->webPage()->tabId());
        index = findTabIndex(newActiveTabId);
    } else {
        --index;
    }
    return index;
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
            updateThumbPath(tabId, path);
        }
    }
}

void DeclarativeTabModel::onUrlChanged()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage) {
        QString url = webPage->url().toString();
        int tabId = webPage->tabId();

        // Initial url should not be considered as navigation request that increases navigation history.
        // Cleanup this.
        bool initialLoad = !webPage->initialLoadHasHappened();
        // Virtualized pages need to be checked from the model.
        if (!initialLoad || contains(tabId)) {
            updateUrl(tabId, url, initialLoad);
        } else {
            // Adding tab to the model is delayed so that url resolved to download link do not get added
            // to the model. We should have downloadStatus(status) and linkClicked(url) signals in QmlMozView.
            // To distinguish linkClicked(url) from downloadStatus(status) the downloadStatus(status) signal
            // should not be emitted when link clicking started downloading or opened (will open) a new window.
            if (webPage->parentId() > 0) {
                int parentTabId = m_webContainer->findParentTabId(tabId);
                addTab(url, "", findTabIndex(parentTabId) + 1);
            } else {
                addTab(url, "", m_tabs.count());
            }
        }
        webPage->setInitialLoadHasHappened();
    }
}

void DeclarativeTabModel::onTitleChanged()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage) {
        QString title = webPage->title();
        int tabId = webPage->tabId();
        int tabIndex = findTabIndex(tabId);
        if (tabIndex >= 0 && (m_tabs.at(tabIndex).title() != title)) {
            QVector<int> roles;
            roles << TitleRole;
            m_tabs[tabIndex].setTitle(title);
            emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
            updateTitle(tabId, m_tabs.at(tabIndex).currentLink(), webPage->url().toString(), title);
        }
    }
}
