/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
** Contact: Petri M. Gerdt <petri.gerdt@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QFile>
#include <QDebug>
#include <QDesktopServices>
#include <QStringList>
#include <QUrl>

#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "declarativetabmodel.h"

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

namespace  {
    bool isExternalUrl(const QUrl &url)
    {
        return url.scheme() == QLatin1String("tel") ||
                url.scheme() == QLatin1String("sms") ||
                url.scheme() == QLatin1String("mailto") ||
                url.scheme() == QLatin1String("geo");
    }
}

DeclarativeTabModel::DeclarativeTabModel(int nextTabId, DeclarativeWebContainer *webContainer)
    : QAbstractListModel(webContainer)
    , m_activeTabId(0)
    , m_loaded(false)
    , m_nextTabId(nextTabId)
    , m_unittestMode(false)
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
    roles[DesktopModeRole] = "desktopMode";
    return roles;
}

void DeclarativeTabModel::addTab(const Tab& tab, int index) {
    Q_ASSERT(index >= 0 && index <= m_tabs.count());
    createTab(tab);

#if DEBUG_LOGS
    qDebug() << "new tab data:" << &tab;
#endif
    beginInsertRows(QModelIndex(), index, index);
    m_tabs.insert(index, tab);
    endInsertRows();
    // We should trigger this only when
    // tab is added through new window request. In all other
    // case we should keep the new tab in background.
    updateActiveTab(tab, false);

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
            newActiveIndex = shiftNewActiveIndex(index, newActiveIndex);
            activateTab(newActiveIndex);
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
}

bool DeclarativeTabModel::activateTab(const QString& url, bool reload)
{
    // Skip empty url
    if (url.isEmpty()) {
        return false;
    }

    QUrl inputUrl(url);
    if (!inputUrl.hasFragment() && !inputUrl.hasQuery() && inputUrl.path().endsWith(QLatin1Char('/'))) {
        QString inputUrlStr = url;
        inputUrlStr.chop(1);
        inputUrl.setUrl(inputUrlStr);
    }

    for (int i = 0; i < m_tabs.size(); i++) {
        const Tab &tab = m_tabs.at(i);
        if (matches(inputUrl, tab.url()) || matches(inputUrl, tab.requestedUrl())) {
            activateTab(i, reload);
            return true;
        }
    }
    return false;
}

void DeclarativeTabModel::activateTab(int index, bool reload)
{
    if (m_tabs.isEmpty()) {
        return;
    }

    index = qBound<int>(0, index, m_tabs.count() - 1);
    const Tab &newActiveTab = m_tabs.at(index);
#if DEBUG_LOGS
    qDebug() << "activate tab: " << index << &newActiveTab;
#endif
    updateActiveTab(newActiveTab, reload);
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
        newActiveIndex = shiftNewActiveIndex(index, newActiveIndex);
        activateTab(newActiveIndex, false);
    }
}

int DeclarativeTabModel::newTab(const QString &url)
{
    return newTab(url, 0, 0);
}

int DeclarativeTabModel::newTab(const QString &url, int parentId, uintptr_t browsingContext)
{
    // When browser opens without tabs
    if ((url.isEmpty() || url == QStringLiteral("about:blank")) && m_tabs.isEmpty())
        return 0;

    QUrl requestedUrl(url, QUrl::TolerantMode);
    if (isExternalUrl(requestedUrl)) {
        if (!m_unittestMode) {
            QDesktopServices::openUrl(requestedUrl);
        }

        return 0;
    }

    Tab tab;
    tab.setTabId(nextTabId());
    tab.setRequestedUrl(url);
    tab.setBrowsingContext(browsingContext);
    tab.setParentId(parentId);

    int index = 0;

    if (parentId > 0) {
        int parentTabId = m_webContainer->tabId((uint32_t)parentId);
        index = findTabIndex(parentTabId) + 1;
        if (index == 0) {
            index = m_tabs.count();
        }
    } else {
        index = m_tabs.count();
    }

    emit newTabRequested(tab);

    addTab(tab, index);

    return tab.tabId();
}

QString DeclarativeTabModel::url(int tabId) const
{
    int index = findTabIndex(tabId);
    if (index >= 0) {
        return m_tabs.at(index).url();
    }
    return "";
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
    if (index.row() < 0 || index.row() >= m_tabs.count())
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
    } else if (role == DesktopModeRole) {
        return tab.desktopMode();
    }
    return QVariant();
}

bool DeclarativeTabModel::loaded() const
{
    return m_loaded;
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

Tab *DeclarativeTabModel::getTab(int tabId)
{
    int index = findTabIndex(tabId);
    if (index >= 0) {
        return &m_tabs[index];
    }

    return nullptr;
}

bool DeclarativeTabModel::contains(int tabId) const
{
    return findTabIndex(tabId) >= 0;
}

void DeclarativeTabModel::updateUrl(int tabId, const QString &url)
{
    QUrl resolvedUrl(url, QUrl::TolerantMode);
    if (isExternalUrl(resolvedUrl)) {
        return;
    }

    int tabIndex = findTabIndex(tabId);
    bool isActiveTab = m_activeTabId == tabId;
    QString requestedUrl;
    if (tabIndex >= 0 && (m_tabs.at(tabIndex).url() != url || isActiveTab)) {
        QVector<int> roles;
        roles << UrlRole;

        Tab &tab = m_tabs[tabIndex];

        bool hadUrl = tab.hasResolvedUrl();
        tab.setUrl(url);
        emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);

        requestedUrl = tab.requestedUrl();

        if (hadUrl || requestedUrl == url) {
            // This is causing navigation i.e. not first urlChanged.
            tab.setRequestedUrl(QString());
        }
    }

    if (!requestedUrl.isEmpty()) {
        updateRequestedUrl(tabId, requestedUrl, url);
    } else {
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

void DeclarativeTabModel::updateActiveTab(const Tab &activeTab, bool reload)
{
#if DEBUG_LOGS
    qDebug() << "new active tab:" << &activeTab << "old active tab:" << m_activeTabId << "count:" << m_tabs.count();
#endif
    if (m_tabs.isEmpty()) {
        return;
    }

    if (m_activeTabId != activeTab.tabId() || reload) {
        int oldTabId = m_activeTabId;
        m_activeTabId = activeTab.tabId();

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
        emit activeTabChanged(activeTab.tabId());
    }
}

void DeclarativeTabModel::setWebContainer(DeclarativeWebContainer *webContainer)
{
    m_webContainer = webContainer;
}

bool DeclarativeTabModel::matches(const QUrl &inputUrl, QString urlStr) const
{
    if (urlStr.isEmpty()) {
        return false;
    }

    QUrl tabUrl(urlStr);
    // Always chop trailing slash if no fragment or query exists as QUrl::StripTrailingSlash
    // doesn't remove trailing slash if path is "/" e.i. http://www.sailfishos.org vs http://www.sailfishos.org/
    if (!tabUrl.hasFragment() && !tabUrl.hasQuery() && tabUrl.path().endsWith(QLatin1Char('/'))) {
        urlStr.chop(1);
        tabUrl.setUrl(urlStr);
    }

    bool match = tabUrl.matches(inputUrl, QUrl::FullyDecoded | QUrl::RemoveScheme | QUrl::StripTrailingSlash);
    // Matching http to https is fine but not other round.
    bool okScheme = tabUrl.scheme() == inputUrl.scheme() ||
            tabUrl.scheme() == QLatin1String("https") ||
            inputUrl.scheme().isEmpty();
    if (!okScheme) {
        return false;
    }

    if (match) {
        return match;
    }

    return inputUrl.matches(tabUrl, QUrl::FullyDecoded | QUrl::RemoveScheme | QUrl::StripTrailingSlash);
}

int DeclarativeTabModel::nextActiveTabIndex(int index)
{
    if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
        uint32_t parentId = m_tabs.at(index).parentId();
        int tabId = 0;
        if (parentId > 0) {
            tabId = m_webContainer->tabId(parentId);
            // Parent tab has been closed, active previously used tab instead.
            if (tabId == 0) {
                tabId = m_webContainer->previouslyUsedTabId();
            }
        } else {
            tabId = m_webContainer->previouslyUsedTabId();
        }
        index = findTabIndex(tabId);
    } else {
        --index;
    }

    return std::clamp(index, 0, std::max(0, m_tabs.count() - 1));
}

int DeclarativeTabModel::shiftNewActiveIndex(int oldIndex, int newIndex)
{
    if (oldIndex < newIndex) {
        --newIndex;
        newIndex = std::clamp(newIndex, 0, std::max(0, m_tabs.count() - 1));
    }
    return newIndex;
}

void DeclarativeTabModel::updateThumbnailPath(int tabId, const QString &path)
{
    if (tabId <= 0)
        return;

    QVector<int> roles;
    roles << ThumbPathRole;
    for (int i = 0; i < m_tabs.count(); i++) {
        if (m_tabs.at(i).tabId() == tabId) {
#if DEBUG_LOGS
            qDebug() << "model tab thumbnail updated: " << path << i << tabId;
#endif
            QModelIndex start = index(i, 0);
            QModelIndex end = index(i, 0);
            m_tabs[i].setThumbnailPath(path);
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
        if (contains(tabId)) {
            updateUrl(tabId, url);
        }
    }
}

void DeclarativeTabModel::onDesktopModeChanged()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage) {
        int tabIndex = findTabIndex(webPage->tabId());
        if (tabIndex >= 0 && m_tabs.at(tabIndex).desktopMode() != webPage->desktopMode()) {
            QVector<int> roles;
            roles << DesktopModeRole;
            m_tabs[tabIndex].setDesktopMode(webPage->desktopMode());
            emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
        }
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
            updateTitle(tabId, webPage->url().toString(), title);
        }
    }
}
