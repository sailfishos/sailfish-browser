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
#include "declarativetab.h"
#include "linkvalidator.h"
#include <QFile>
#include <QDebug>
#include <QStringList>

static QList<int> s_tabOrder;

DeclarativeTabModel::DeclarativeTabModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_currentTab(0)
    , m_loaded(false)
    , m_browsing(false)
    , m_nextTabId(DBManager::instance()->getMaxTabId() + 1)
    , m_backForwardNavigation(false)
{
    connect(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)),
            this, SLOT(tabsAvailable(QList<Tab>)));
    connect(DBManager::instance(), SIGNAL(tabChanged(Tab)),
            this, SLOT(tabChanged(Tab)));
    connect(DBManager::instance(), SIGNAL(thumbPathChanged(QString,QString,int)),
            this, SLOT(updateThumbPath(QString,QString,int)));
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
    if (!LinkValidator::navigable(url)) {
        return;
    }
    int tabId = DBManager::instance()->createTab();
    int linkId = DBManager::instance()->createLink(tabId, url, title);

    Tab tab(tabId, Link(linkId, url, "", title), 0, 0);
#ifdef DEBUG_LOGS
    qDebug() << "new tab data:" << &tab;
#endif
    if (m_currentTab && m_currentTab->valid()) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_tabs.insert(0, m_currentTab->tabData());
        endInsertRows();
    }

    updateActiveTab(tab);
    emit countChanged();
    emit tabAdded(tabId);

    m_nextTabId = ++tabId;
    emit nextTabIdChanged();
}

int DeclarativeTabModel::currentTabId() const
{
    if (m_currentTab && m_currentTab->valid()) {
        return m_currentTab->tabId();
    }
    return 0;
}

int DeclarativeTabModel::nextTabId() const
{
    return m_nextTabId;
}

void DeclarativeTabModel::remove(const int index) {
    if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
        beginRemoveRows(QModelIndex(), index, index);
        removeTab(m_tabs.at(index).tabId(), m_tabs.at(index).currentLink().thumbPath(), index);
        endRemoveRows();
        saveTabOrder();
    }
}

void DeclarativeTabModel::removeTabById(const int &tabId)
{
    bool isActiveTab = false;
    int index = findTabIndex(tabId, isActiveTab);

    if (isActiveTab) {
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
        removeTab(m_tabs.at(i).tabId(), m_tabs.at(i).currentLink().thumbPath(), i);
    }
    closeActiveTab();
    endResetModel();
}


bool DeclarativeTabModel::activateTab(const QString& url)
{
    if (m_currentTab && m_currentTab->url() == url) {
        return true;
    }
    for (int i = 0; i < m_tabs.size(); i++) {
        if (m_tabs.at(i).currentLink().url() == url) {
            return activateTab(i);
        }
    }
    return false;
}

bool DeclarativeTabModel::activateTab(const int &index)
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
        if (m_currentTab && m_currentTab->valid()) {
#ifdef DEBUG_LOGS
            qDebug() << "insert to first index: " << m_currentTab;
#endif
            beginInsertRows(QModelIndex(), 0, 0);
            m_tabs.insert(0, m_currentTab->tabData());
            endInsertRows();
        }

        updateActiveTab(newActiveTab);
        return true;
    }
    return false;
}

void DeclarativeTabModel::activateTabById(const int &tabId)
{
    bool isActiveTab = false;
    int index = findTabIndex(tabId, isActiveTab);

    if (!isActiveTab && index >= 0) {
        activateTab(index);
    }
}

void DeclarativeTabModel::closeActiveTab()
{
    if (m_currentTab && m_currentTab->valid()) {
#ifdef DEBUG_LOGS
        qDebug() << m_currentTab;
#endif
        // Clear active tab data and try to active a tab from the first model index.
        int activeTabId = m_currentTab->tabId();
        m_currentTab->setInvalid();
        removeTab(activeTabId, m_currentTab->thumbnailPath());
        if (!activateTab(0)) {
            // Last active tab got closed.
            emit _activeTabInvalidated();
            emit activeTabChanged(0);
        }
    }
}

int DeclarativeTabModel::lastTabId() const
{
    return m_tabs.at(m_tabs.count() - 1).tabId();
}

void DeclarativeTabModel::dumpTabs() const
{
    if (m_currentTab && m_currentTab->valid()) {
        qDebug() << "active tab:" << m_currentTab;
    }

    for (int i = 0; i < m_tabs.size(); i++) {
        qDebug() << "tab[" << i << "]:" << &m_tabs.at(i);
    }
}

int DeclarativeTabModel::count() const
{
    if (m_currentTab && m_currentTab->valid()) {
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
        return tab.currentLink().thumbPath();
    } else if (role == TitleRole) {
        return tab.currentLink().title();
    } else if (role == UrlRole) {
        return tab.currentLink().url();
    } else if (role == TabIdRole) {
        return tab.tabId();
    }
    return QVariant();
}

void DeclarativeTabModel::componentComplete()
{
}

void DeclarativeTabModel::setCurrentTab(DeclarativeTab *currentTab)
{
    if (currentTab != m_currentTab) {
        if (m_currentTab) {
            m_currentTab->disconnect(this);
        }

        m_currentTab = currentTab;
        if (m_currentTab) {
            connect(m_currentTab, SIGNAL(thumbPathChanged(QString,int)), this, SLOT(updateThumbPath(QString,int)));
        }

        emit currentTabChanged();
    }
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
        if (browsing && m_currentTab && m_currentTab->valid()) {
            QFile f(m_currentTab->thumbnailPath());
            if (f.exists()) {
                f.remove();
            }
            f.close();
            m_currentTab->setThumbnailPath("");
        }

        m_browsing = browsing;
        emit browsingChanged();
    }
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
    emit _activeTabInvalidated();

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
    if (!m_currentTab) {
        qWarning() << "CurrentTab not ready!: " << m_currentTab;
        return;
    }

#ifdef DEBUG_LOGS
    qDebug() << m_currentTab;
    qDebug() << "new tab data:" << &tab;
#endif
    if (m_currentTab && m_currentTab->tabId() == tab.tabId()) {
        updateActiveTab(tab);
    } else {
        int i = m_tabs.indexOf(tab); // match based on tab_id
        if (i > -1) {
            QVector<int> roles;
            Tab oldTab = m_tabs[i];
            if (oldTab.currentLink().url() != tab.currentLink().url()) {
                roles << UrlRole;
            }
            if (oldTab.currentLink().title() != tab.currentLink().title()) {
                roles << TitleRole;
            }
            if (oldTab.currentLink().thumbPath() != tab.currentLink().thumbPath()) {
                roles << ThumbPathRole;
            }
            m_tabs[i] = tab;
            QModelIndex start = index(i, 0);
            QModelIndex end = index(i, 0);
            emit dataChanged(start, end, roles);
        }
    }
}

void DeclarativeTabModel::updateUrl(int tabId, QString url)
{
    if (m_currentTab && m_backForwardNavigation && m_currentTab->tabId() == tabId)
    {
        updateTabUrl(tabId, url, false);
    } else {
        QVariant hasNewTabDataVar = property("hasNewTabData");
        bool hasNewTabData = hasNewTabDataVar.isValid() && hasNewTabDataVar.toBool();
        if (!hasNewTabData && m_currentTab) {
            updateTabUrl(tabId, url, true);
        } else {
            QString newTabTitle = property("newTabTitle").toString();
            addTab(url, newTabTitle);
        }
    }
}

void DeclarativeTabModel::updateTitle(int tabId, QString title)
{
    bool isActiveTab = false;
    int tabIndex = findTabIndex(tabId, isActiveTab);

    bool updateDb = false;
    int linkId = -1;
    if (isActiveTab && m_currentTab->title() != title) {
        linkId = m_currentTab->linkId();
        m_currentTab->setTitle(title);
        updateDb = true;
    } else if (tabIndex >= 0) {
        Link link = m_tabs.at(tabIndex).currentLink();
        if (link.title() != title) {
            linkId = link.linkId();
            link.setTitle(title);
            QVector<int> roles;
            roles << TitleRole;
            m_tabs[tabIndex].setCurrentLink(link);
            emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
            updateDb = true;
        }
    }

    if (updateDb) {
        DBManager::instance()->updateTitle(linkId, title);
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

int DeclarativeTabModel::findTabIndex(int tabId, bool &activeTab) const
{
    if (m_currentTab && m_currentTab->tabId() == tabId) {
        activeTab = true;
        return -1;
    } else {
        activeTab = false;
        for (int i = 0; i < m_tabs.size(); i++) {
            if (m_tabs.at(i).tabId() == tabId) {
                return i;
            }
        }
        return -1;
    }
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
    if (m_currentTab) {
#ifdef DEBUG_LOGS
        qDebug() << "active tab:" << m_currentTab;
#endif
        DBManager::instance()->saveSetting("activeTab", QString("%1").arg(m_currentTab->tabId()));
    }
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

void DeclarativeTabModel::updateActiveTab(const Tab &newActiveTab)
{
    if (!m_currentTab) {
        qWarning() << "CurrentTab not ready!: " << m_currentTab;
        return;
    }

#ifdef DEBUG_LOGS
    qDebug() << "old active tab: " << m_currentTab << m_tabs.count();
    qDebug() << "new active tab: " << &newActiveTab;
#endif
    int activeTabId = m_currentTab->tabId();
    emit _activeTabChanged(newActiveTab);
    if (activeTabId != newActiveTab.tabId()) {
        emit currentTabIdChanged();
        emit activeTabChanged(newActiveTab.tabId());
        saveTabOrder();
    }
}

void DeclarativeTabModel::updateTabUrl(int tabId, const QString &url, bool navigate)
{
    if (!LinkValidator::navigable(url)) {
#ifdef DEBUG_LOGS
        qDebug() << "invalid url: " << url;
#endif
        return;
    }

    bool isActiveTab = false;
    int tabIndex = findTabIndex(tabId, isActiveTab);

    bool updateDb = false;
    if (isActiveTab) {
        m_currentTab->setUrl(url);
        m_currentTab->setTitle("");
        m_currentTab->setThumbnailPath("");
        updateDb = true;
    } else if (tabIndex >= 0) {
        Link link = m_tabs.at(tabIndex).currentLink();
        if (link.url() != url) {
            link.setUrl(url);
            link.setTitle("");
            link.setThumbPath("");
            QVector<int> roles;
            roles << UrlRole << ThumbPathRole;
            m_tabs[tabIndex].setCurrentLink(link);
            emit dataChanged(index(tabIndex, 0), index(tabIndex, 0), roles);
            updateDb = true;
        }
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

void DeclarativeTabModel::updateThumbPath(QString path, int tabId)
{
    // TODO: Remove url parameter from this, db worker, and db manager.
    updateThumbPath("", path, tabId);
}

void DeclarativeTabModel::updateThumbPath(QString url, QString path, int tabId)
{
    Q_UNUSED(url)
#ifdef DEBUG_LOGS
    qDebug() << m_currentTab;
#endif
    if (m_currentTab && m_currentTab->tabId() == tabId) {
        m_currentTab->setThumbnailPath(path);
    } else {
        QVector<int> roles;
        roles << ThumbPathRole;
        for (int i = 0; i < m_tabs.count(); i++) {
            if (m_tabs.at(i).tabId() == tabId &&
                    m_tabs.at(i).currentLink().thumbPath() != path) {
#ifdef DEBUG_LOGS
                qDebug() << "model tab thumbnail updated: " << path << tabId;
#endif
                Link link = m_tabs.at(i).currentLink();
                link.setThumbPath(path);
                m_tabs[i].setCurrentLink(link);
                QModelIndex start = index(i, 0);
                QModelIndex end = index(i, 0);
                emit dataChanged(start, end, roles);
            }
        }
    }
}
