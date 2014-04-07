/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativetabmodel.h"
#include "dbmanager.h"
#include "linkvalidator.h"

#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QUrl>

static QList<int> s_tabOrder;

DeclarativeTabModel::DeclarativeTabModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_loaded(false)
    , m_browsing(false)
    , m_nextTabId(DBManager::instance()->getMaxTabId() + 1)
    , m_backForwardNavigation(false)
{
    connect(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)),
            this, SLOT(tabsAvailable(QList<Tab>)));
    connect(DBManager::instance(), SIGNAL(tabChanged(Tab)),
            this, SLOT(tabChanged(Tab)));
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
#ifdef DEBUG_LOGS
    qDebug() << "new tab data:" << &tab;
#endif
    if (m_activeTab.isValid()) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_tabs.insert(0, m_activeTab);
        endInsertRows();
    }

    updateActiveTab(tab);
    emit countChanged();
    emit tabAdded(tabId);

    m_nextTabId = ++tabId;
    emit nextTabIdChanged();
}

int DeclarativeTabModel::nextTabId() const
{
    return m_nextTabId;
}

void DeclarativeTabModel::remove(int index) {
    if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
        beginRemoveRows(QModelIndex(), index, index);
        removeTab(m_tabs.at(index).tabId(), m_tabs.at(index).thumbnailPath(), index);
        endRemoveRows();
        saveTabOrder();
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

    beginResetModel();
    for (int i = m_tabs.count() - 1; i >= 0; --i) {
        removeTab(m_tabs.at(i).tabId(), m_tabs.at(i).thumbnailPath(), i);
    }
    closeActiveTab();
    endResetModel();
}


bool DeclarativeTabModel::activateTab(const QString& url)
{
    if (m_activeTab.url() == url) {
        return true;
    }
    for (int i = 0; i < m_tabs.size(); i++) {
        if (m_tabs.at(i).url() == url) {
            return activateTab(i);
        }
    }
    return false;
}

bool DeclarativeTabModel::activateTab(int index)
{
    if (index >= 0 && index < m_tabs.count()) {
        Tab newActiveTab = m_tabs.at(index);
#ifdef DEBUG_LOGS
        qDebug() << "activate tab: " << index << &newActiveTab;
#endif
        beginRemoveRows(QModelIndex(), index, index);
        m_tabs.removeAt(index);
        endRemoveRows();

        // Current active tab back to model data.
        if (m_activeTab.isValid()) {
#ifdef DEBUG_LOGS
            qDebug() << "insert to first index: " << &m_activeTab;
#endif
            beginInsertRows(QModelIndex(), 0, 0);
            m_tabs.insert(0, m_activeTab);
            endInsertRows();
        }

        updateActiveTab(newActiveTab);
        return true;
    }
    return false;
}

bool DeclarativeTabModel::activateTabById(int tabId)
{
    int index = findTabIndex(tabId);
    if (index >= 0) {
        return activateTab(index);
    }
    return false;
}

void DeclarativeTabModel::closeActiveTab()
{
    if (m_activeTab.isValid()) {
#ifdef DEBUG_LOGS
        qDebug() << &m_activeTab;
#endif
        // Clear active tab data and try to active a tab from the first model index.
        int activeTabId = m_activeTab.tabId();
        // Invalidate
        m_activeTab.setTabId(0);
        removeTab(activeTabId, m_activeTab.thumbnailPath());
        if (!activateTab(0)) {
            // Last active tab got closed.
            emit activeTabChanged(activeTabId, 0);
        }
    }
}

void DeclarativeTabModel::newTab(const QString &url, const QString &title, int parentId)
{
    emit newTabRequested(url, title, parentId);
}

void DeclarativeTabModel::newTabData(const QString &url, const QString &title, QObject *contentItem, int parentId)
{
    updateNewTabData(new NewTabData(url, title, contentItem, parentId));
}

void DeclarativeTabModel::resetNewTabData()
{
    updateNewTabData(0);
}

void DeclarativeTabModel::dumpTabs() const
{
    if (m_activeTab.isValid()) {
        qDebug() << "active tab:" << &m_activeTab;
    }

    for (int i = 0; i < m_tabs.size(); i++) {
        qDebug() << "tab[" << i << "]:" << &m_tabs.at(i);
    }
}

int DeclarativeTabModel::count() const
{
    if (m_activeTab.isValid()) {
        return m_tabs.count() + 1;
    }
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

bool DeclarativeTabModel::browsing() const
{
    return m_browsing;
}

void DeclarativeTabModel::setBrowsing(bool browsing)
{
    if (browsing != m_browsing) {
        if (browsing && m_activeTab.isValid()) {
            QFile f(m_activeTab.thumbnailPath());
            if (f.exists()) {
                f.remove();
            }
            f.close();
            emit updateActiveTabThumbnail("");
            m_activeTab.setThumbnailPath("");
        }

        m_browsing = browsing;
        emit browsingChanged();
    }
}

bool DeclarativeTabModel::hasNewTabData() const
{
    return m_newTabData && !m_newTabData->url.isEmpty();
}

QString DeclarativeTabModel::newTabUrl() const
{
    return hasNewTabData() ? m_newTabData->url : "";
}

QString DeclarativeTabModel::newTabTitle() const
{
    return hasNewTabData() ? m_newTabData->title : "";
}

QObject *DeclarativeTabModel::newTabPreviousPage() const
{
    return hasNewTabData() ? m_newTabData->previousPage : 0;
}

const QList<Tab> &DeclarativeTabModel::tabs() const
{
    return m_tabs;
}

const Tab &DeclarativeTabModel::activeTab() const
{
    return m_activeTab;
}

int DeclarativeTabModel::newTabParentId() const
{
    return hasNewTabData() ? m_newTabData->parentId : 0;
}

bool DeclarativeTabModel::backForwardNavigation() const
{
    return m_backForwardNavigation;
}

void DeclarativeTabModel::setBackForwardNavigation(bool backForwardNavigation)
{
    m_backForwardNavigation = backForwardNavigation;
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

    int activeTabId = loadTabOrder();
    if (m_tabs.count() > 0) {
        Tab tab;
        tab.setTabId(activeTabId);

        int index = m_tabs.indexOf(tab);
        if (index == -1) {
            index = 0;
        }
        const Tab &activeTab = m_tabs.at(index);
        m_tabs.removeAt(index);
        updateActiveTab(activeTab);
    }

    qSort(m_tabs.begin(), m_tabs.end(), DeclarativeTabModel::tabSort);
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
#ifdef DEBUG_LOGS
    qDebug() << &m_activeTab;
    qDebug() << "new tab data:" << &tab;
#endif
    if (m_activeTab.tabId() == tab.tabId()) {
        updateActiveTab(tab);
    } else {
        int i = m_tabs.indexOf(tab); // match based on tab_id
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
    }
}

void DeclarativeTabModel::updateUrl(int tabId, bool activeTab, QString url)
{
    if (m_backForwardNavigation && activeTab)
    {
        updateTabUrl(tabId, activeTab, url, false);
    } else if (!hasNewTabData() && activeTab) {
        updateTabUrl(tabId, activeTab, url, true);
    } else {
        addTab(url, newTabTitle());
    }
    resetNewTabData();
}

void DeclarativeTabModel::updateTitle(int tabId, bool activeTab, QString title)
{
    int tabIndex = findTabIndex(tabId);

    bool updateDb = false;

    if (activeTab && m_activeTab.title() != title) {
        m_activeTab.setTitle(title);
        updateDb = true;
    } else if (tabIndex >= 0 && m_tabs.at(tabIndex).title() != title) {
        QVector<int> roles;
        roles << TitleRole;
        m_tabs[tabIndex].setTitle(title);
        emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
        updateDb = true;
    }

    if (updateDb) {
        DBManager::instance()->updateTitle(m_activeTab.currentLink(), title);
    }
}

void DeclarativeTabModel::removeTab(int tabId, const QString &thumbnail, int index)
{
#ifdef DEBUG_LOGS
    qDebug() << "index:" << index << tabId;
#endif
    DBManager::instance()->removeTab(tabId);
    QFile f(thumbnail);
    if (f.exists()) {
        f.remove();
    }

    if (index >= 0) {
        m_tabs.removeAt(index);
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

void DeclarativeTabModel::saveTabOrder()
{
    QString tabOrder = "";
    for (int i = 0; i < m_tabs.count(); ++i) {
        const Tab &tab = m_tabs.at(i);
        tabOrder.append(QString("%1,").arg(tab.tabId()));
#ifdef DEBUG_LOGS
        qDebug() << "append:" << &tab;
#endif
    }
    DBManager::instance()->saveSetting("tabOrder", tabOrder);
    DBManager::instance()->saveSetting("activeTab", QString("%1").arg(m_activeTab.tabId()));
}

int DeclarativeTabModel::loadTabOrder()
{
    QString tabOrder = DBManager::instance()->getSetting("tabOrder");
    QStringList tmpList = tabOrder.split(",");
    bool ok = false;
    for (int i = 0; i < tmpList.count(); ++i) {
        const QString &strTab = tmpList.at(i);
        int tabId = strTab.toInt(&ok);
        if (ok) {
            s_tabOrder << tabId;
        }
    }

    QString activeTab = DBManager::instance()->getSetting("activeTab");
    int activeTabId = activeTab.toInt(&ok);
#ifdef DEBUG_LOGS
    qDebug() << "loaded tab order:" << s_tabOrder << "active tab: " << activeTabId;
#endif
    if (ok) {
        return activeTabId;
    } else {
        return 0;
    }
}

void DeclarativeTabModel::updateActiveTab(const Tab &activeTab)
{
#ifdef DEBUG_LOGS
    qDebug() << "old active tab: " << &m_activeTab << m_tabs.count();
    qDebug() << "new active tab: " << &activeTab;
#endif
    if (m_activeTab != activeTab) {
        int oldTabId = m_activeTab.tabId();
        m_activeTab = activeTab;
        emit activeTabChanged(oldTabId, activeTab.tabId());
        saveTabOrder();
    }
}

void DeclarativeTabModel::updateTabUrl(int tabId, bool activeTab, const QString &url, bool navigate)
{
    if (!LinkValidator::navigable(QUrl(url))) {
#ifdef DEBUG_LOGS
        qDebug() << "invalid url: " << url;
#endif
        return;
    }

    int tabIndex = findTabIndex(tabId);
    bool updateDb = false;
    if (activeTab) {
        m_activeTab.setUrl(url);
        updateDb = true;
    } else if (tabIndex >= 0 && m_tabs.at(tabIndex).url() != url) {
        QVector<int> roles;
        roles << UrlRole << TitleRole << ThumbPathRole;
        m_tabs[tabIndex].setUrl(url);
        m_tabs[tabIndex].setTitle("");
        m_tabs[tabIndex].setThumbnailPath("");
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

    setBackForwardNavigation(false);
}

void DeclarativeTabModel::updateNewTabData(NewTabData *newTabData)
{
    bool hadNewTabData = hasNewTabData();
    QString currentTabUrl = newTabUrl();
    bool urlChanged = newTabData ? currentTabUrl != newTabData->url : !currentTabUrl.isEmpty();

    m_newTabData.reset(newTabData);
    if (urlChanged) {
        emit newTabUrlChanged();
    }

    if (hadNewTabData != hasNewTabData()) {
        emit hasNewTabDataChanged();
    }
}

bool DeclarativeTabModel::tabSort(const Tab &t1, const Tab &t2)
{
    int i1 = s_tabOrder.indexOf(t1.tabId());
    int i2 = s_tabOrder.indexOf(t2.tabId());
    if (i2 == -1) {
        return true;
    } else {
        return i1 < i2;
    }
}

void DeclarativeTabModel::updateThumbnailPath(int tabId, bool activeTab, QString path)
{
#ifdef DEBUG_LOGS
    qDebug() << &m_activeTab;
#endif
    if (activeTab) {
        m_activeTab.setThumbnailPath(path);
    } else {
        QVector<int> roles;
        roles << ThumbPathRole;
        for (int i = 0; i < m_tabs.count(); i++) {
            if (m_tabs.at(i).tabId() == tabId && m_tabs.at(i).thumbnailPath() != path) {
#ifdef DEBUG_LOGS
                qDebug() << "model tab thumbnail updated: " << path << tabId;
#endif
                m_tabs[i].setThumbnailPath(path);
                QModelIndex start = index(i, 0);
                QModelIndex end = index(i, 0);
                emit dataChanged(start, end, roles);
            }
        }
    }
}
