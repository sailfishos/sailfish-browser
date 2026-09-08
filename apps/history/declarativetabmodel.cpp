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
#include "declarativetabmodel.h"

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

namespace  {
    bool isExternalUrl(const QUrl &url)
    {
        return url.scheme() == QLatin1String("tel")
                || url.scheme() == QLatin1String("sms")
                || url.scheme() == QLatin1String("mailto")
                || url.scheme() == QLatin1String("geo");
    }
}

DeclarativeTabModel::DeclarativeTabModel(int nextTabId, DeclarativeWebContainer *webContainer)
    : QAbstractListModel(webContainer)
    , m_activeTabId(0)
    , m_loaded(false)
    , m_nextTabId(nextTabId)
    , m_unittestMode(false)
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
    roles[HiddenRole] = "hidden";
    return roles;
}

int DeclarativeTabModel::nextTabId() const
{
    return m_nextTabId;
}

void DeclarativeTabModel::remove(int index)
{
    if (index >= 0 && index < m_tabs.count()) {
        emit runtimeTabCloseRequested(QString::number(m_tabs.at(index).tabId()));
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
    if (count() > 0) {
        emit runtimeTabsClearRequested();
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
    emit runtimeTabActivationRequested(QString::number(newActiveTab.tabId()), reload);
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

bool DeclarativeTabModel::requestRuntimeTabNavigation(
        int tabId, const QString &url, bool fromExternal)
{
    if (url.isEmpty() || !contains(tabId)) {
        return false;
    }
    emit runtimeTabNavigationRequested(QString::number(tabId), url, fromExternal);
    return true;
}

bool DeclarativeTabModel::runtimeNavigateTab(
        const QString &persistentId, const QString &url, bool fromExternal)
{
    bool ok = false;
    const int tabId = persistentId.toInt(&ok);
    return ok && requestRuntimeTabNavigation(tabId, url, fromExternal);
}

/**
 * @brief DeclarativeTabModel::closeActiveTab
 * Closes the active tab and activates a tab next to the current tab. If possible
 * tab that is after the current tab is activated, then falling back to previous tabs, or
 * finally none (if all closed).
 */
void DeclarativeTabModel::closeActiveTab()
{
    if (contains(m_activeTabId)) {
        emit runtimeTabCloseRequested(QString::number(m_activeTabId));
    }
}

int DeclarativeTabModel::newTab(const QString &url, bool fromExternal)
{
    return newTab(url, 0, 0, false, fromExternal);
}

int DeclarativeTabModel::newTab(const QString &url, int parentId, uintptr_t browsingContext, bool hidden, bool fromExternal)
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

    Tab tab(nextTabId(), url, QString(), QString(), hidden);
    tab.setBrowsingContext(browsingContext);
    tab.setParentId(parentId);

    createTab(tab);
    ++m_nextTabId;
    emit runtimeNewTabRequested(url, QString::number(tab.tabId()), fromExternal);
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

int DeclarativeTabModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return m_tabs.count();
}

QVariant DeclarativeTabModel::data(const QModelIndex & index, int role) const
{
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
    } else if (role == HiddenRole) {
        return tab.hidden();
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

int DeclarativeTabModel::findTabIndex(int tabId) const
{
    for (int i = 0; i < m_tabs.size(); i++) {
        if (m_tabs.at(i).tabId() == tabId) {
            return i;
        }
    }
    return -1;
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
            if (m_tabs.at(i).thumbnailPath() != path
                    && !m_tabs.at(i).thumbnailPath().isEmpty()) {
                QFile::remove(m_tabs.at(i).thumbnailPath());
            }
            m_tabs[i].setThumbnailPath(path);
            emit dataChanged(start, end, roles);
            updateThumbPath(tabId, path);
        }
    }
}
