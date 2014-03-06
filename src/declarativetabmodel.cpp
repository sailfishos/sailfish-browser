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
#ifdef DEBUG_LOGS
#include <QDebug>
#endif

static QList<int> s_tabOrder;

DeclarativeTabModel::DeclarativeTabModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_currentTab(0)
    , m_loaded(false)
    , m_browsing(false)
    , m_activeTabClosed(false)
    , m_navigated(false)
{
    connect(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)),
            this, SLOT(tabsAvailable(QList<Tab>)));

    connect(DBManager::instance(), SIGNAL(tabChanged(Tab)),
            this, SLOT(tabChanged(Tab)));
    connect(DBManager::instance(), SIGNAL(navigated(Tab)),
            this, SLOT(navigated(Tab)));

    connect(DBManager::instance(), SIGNAL(thumbPathChanged(QString,QString,int)),
            this, SLOT(updateThumbPath(QString,QString,int)));
    connect(DBManager::instance(), SIGNAL(titleChanged(QString,QString)),
            this, SLOT(updateTitle(QString,QString)));
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

// TODO : Remove foreground flag.
void DeclarativeTabModel::addTab(const QString& url, bool foreground) {
    if (!LinkValidator::navigable(url)) {
        return;
    }

    int tabId = DBManager::instance()->createTab();
    int linkId = DBManager::instance()->createLink(tabId, url);
#ifdef DEBUG_LOGS
    qDebug() << "new tab id:" << tabId << "new link id:" << linkId;
#endif
    Tab tab(tabId, Link(linkId, url, "", ""), 0, 0);
    if (m_activeTab.isValid()) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_tabs.insert(0, m_activeTab);
        endInsertRows();
    }

    updateActiveTab(tab, foreground);
    emit countChanged();
}

int DeclarativeTabModel::currentTabId() const
{
    if (!m_tabs.isEmpty()) {
        return m_activeTab.tabId();
    }
    return 0;
}

void DeclarativeTabModel::remove(const int index) {
    if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
        beginRemoveRows(QModelIndex(), index, index);
        removeTab(m_tabs.at(index), index);
        emit countChanged();
        endRemoveRows();
        saveTabOrder();
    }
}

void DeclarativeTabModel::clear()
{
    if (count() == 0)
        return;

    beginResetModel();
    for (int i = m_tabs.count() - 1; i >= 0; --i) {
        removeTab(m_tabs.at(i), i);
    }
    emit countChanged();
    closeActiveTab();
    endResetModel();
    // No need guard anything as all tabs got closed.
    m_activeTabClosed = false;
}


bool DeclarativeTabModel::activateTab(const QString& url)
{
    if (m_activeTab.currentLink().url() == url && m_currentTab) {
        m_currentTab->tabChanged(m_activeTab);
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
        qDebug() << "active tab: " << index << newActiveTab.currentLink().url();
#endif
        beginRemoveRows(QModelIndex(), index, index);
        m_tabs.removeAt(index);
        endRemoveRows();

        // Current active tab back to model data.
        if (m_activeTab.isValid()) {
#ifdef DEBUG_LOGS
            qDebug() << "insert to first index: " << m_activeTab.currentLink().url() << m_activeTab.currentLink().title() << m_activeTab.currentLink().thumbPath();
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

void DeclarativeTabModel::closeActiveTab()
{
#ifdef DEBUG_LOGS
    qDebug() << m_activeTab.isValid() << m_activeTab.tabId() << m_activeTab.currentLink().thumbPath() << m_activeTab.currentLink().url();
#endif
    if (m_activeTab.isValid()) {
        // Invalidate active tab
        removeTab(m_activeTab);
        m_activeTab.setTabId(0);
        emit countChanged();
        m_activeTabClosed = true;
        if (!activateTab(0) && m_currentTab) {
            // Last active tab got closed.
            Link emptyLink;
            m_activeTab.setCurrentLink(emptyLink);
            m_currentTab->invalidate();
        }
    }
}

void DeclarativeTabModel::dumpTabs() const
{
    if (m_activeTab.isValid()) {
        qDebug() << "active tab id:" << m_activeTab.tabId() << "valid:" << m_activeTab.isValid() << "url:" << m_activeTab.currentLink().url() << "title:" << m_activeTab.currentLink().title() << "thumb:" << m_activeTab.currentLink().thumbPath();
    }

    for (int i = 0; i < m_tabs.size(); i++) {
        qDebug() << "tab[" << i << "] id:" << m_tabs.at(i).tabId() << "valid:" << m_tabs.at(i).isValid() << "url:" << m_tabs.at(i).currentLink().url() << "title:" << m_tabs.at(i).currentLink().title() << "thumb:" << m_tabs.at(i).currentLink().thumbPath();
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

DeclarativeTab *DeclarativeTabModel::currentTab() const
{
    return m_currentTab;
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
            connect(m_currentTab, SIGNAL(navigated(QString)), this, SLOT(handleNavigation(QString)));
            connect(m_currentTab, SIGNAL(titleUpdated(QString)), this, SLOT(handleTitleUpdate(QString)));
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
        if (browsing && m_currentTab && m_activeTab.isValid()) {
            m_currentTab->updateThumbPath(m_activeTab.currentLink().url(), "", m_activeTab.tabId());
            QFile f(m_activeTab.currentLink().thumbPath());
            if (f.exists()) {
                f.remove();
            }
            f.close();
        }

        m_browsing = browsing;
        emit browsingChanged();
    }
}

void DeclarativeTabModel::classBegin()
{
    DBManager::instance()->getAllTabs();
}

void DeclarativeTabModel::tabsAvailable(QList<Tab> tabs)
{
    beginResetModel();
    int oldCount = count();
    m_activeTab.setTabId(0);
    m_tabs.clear();
    m_tabs = tabs;
    if (m_currentTab) {
        m_currentTab->invalidate();
    }

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
        updateActiveTab(activeTab, true);
    }

    qSort(m_tabs.begin(), m_tabs.end(), DeclarativeTabModel::tabSort);
    endResetModel();

    // Startup should be synced to this.
    if (!m_loaded) {
        m_loaded = true;
        emit loadedChanged();
    }

    if (count() != oldCount) {
        emit countChanged();
    }
}

void DeclarativeTabModel::tabChanged(Tab tab)
{
    // When a tab was closed do not update anything from database as
    // loading might be on going.
    if (this->sender() == DBManager::instance()) {
        if (m_activeTabClosed) {
            m_activeTabClosed = false;
            return;
        }

        if (m_navigated) {
            return;
        }
    }

#ifdef DEBUG_LOGS
    qDebug() << "tab: " << tab.tabId() << m_activeTab.tabId() << tab.currentLink().thumbPath() << tab.currentLink().url() << tab.currentLink().title() << m_tabs.indexOf(tab);
#endif
    if (tab.tabId() == m_activeTab.tabId()) {
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

void DeclarativeTabModel::handleNavigation(QString url)
{
    Link currentLink = m_activeTab.currentLink();
    currentLink.setTitle("");
    currentLink.setThumbPath("");
    currentLink.setUrl(url);
    m_activeTab.setCurrentLink(currentLink);
    m_navigated = true;
}

void DeclarativeTabModel::handleTitleUpdate(QString title)
{
    Link currentLink = m_activeTab.currentLink();
    currentLink.setTitle(title);
    m_activeTab.setCurrentLink(currentLink);
}

void DeclarativeTabModel::navigated(Tab tab)
{
    if (tab.tabId() == m_activeTab.tabId()) {
        updateActiveTab(tab);
    }
    m_navigated = false;
}


void DeclarativeTabModel::removeTab(const Tab &tab, int index)
{
#ifdef DEBUG_LOGS
    qDebug() << "index:" << index << tab.currentLink().url();
#endif
    int tabId = tab.tabId();
    DBManager::instance()->removeTab(tabId);

    QFile f(tab.currentLink().thumbPath());
    if (f.exists()) {
        f.remove();
    }

    if (index >= 0) {
        m_tabs.removeAt(index);
    }
}

void DeclarativeTabModel::saveTabOrder()
{
    QString tabOrder = "";
    for (int i = 0; i < m_tabs.count(); ++i) {
        const Tab &tab = m_tabs.at(i);
        tabOrder.append(QString("%1,").arg(tab.tabId()));
#ifdef DEBUG_LOGS
        qDebug() << "append: " << tab.tabId() << tab.currentLink().url() << tab.currentLink().title() << tab.currentLink().thumbPath();
#endif
    }
    DBManager::instance()->saveSetting("tabOrder", tabOrder);
#ifdef DEBUG_LOGS
    qDebug() << "active tab:" << QString("%1").arg(m_activeTab.tabId());
#endif
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

void DeclarativeTabModel::updateActiveTab(const Tab &newActiveTab, bool updateCurrentTab)
{
#ifdef DEBUG_LOGS
    qDebug() << "change tab: " << updateCurrentTab << m_currentTab;
    qDebug() << "old active tab: " << m_activeTab.tabId() << m_activeTab.isValid() << m_activeTab.currentLink().url() << m_tabs.count();
    qDebug() << "new active tab: " << newActiveTab.tabId() << newActiveTab.isValid() << newActiveTab.currentLink().url();
#endif
    m_activeTab = newActiveTab;
    emit currentTabIdChanged();

    saveTabOrder();
    if (updateCurrentTab && m_currentTab) {
        m_currentTab->tabChanged(m_activeTab);
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

void DeclarativeTabModel::updateThumbPath(QString path, int tabId)
{
    // TODO: Remove url parameter from this, db worker, and db manager.
    updateThumbPath("", path, tabId);
}

void DeclarativeTabModel::updateThumbPath(QString url, QString path, int tabId)
{
    Q_UNUSED(url)
    if (tabId != m_activeTab.tabId()) {
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
    } else {
        Link link = m_activeTab.currentLink();
        link.setThumbPath(path);
        m_activeTab.setCurrentLink(link);
    }
}

void DeclarativeTabModel::updateTitle(QString url, QString title)
{
    if (m_activeTab.currentLink().url() == url
            && m_activeTab.currentLink().title() != title) {
        Link link = m_activeTab.currentLink();
        link.setTitle(title);
        m_activeTab.setCurrentLink(link);
    }

    QVector<int> roles;
    roles << TitleRole;
    for (int i = 0; i < m_tabs.count(); i++) {
        if (m_tabs.at(i).currentLink().url() == url) {
            if (m_tabs.at(i).currentLink().title() != title) {
                Link tmp = m_tabs[i].currentLink();
                tmp.setTitle(title);
                m_tabs[i].setCurrentLink(tmp);
                QModelIndex start = index(i, 0);
                QModelIndex end = index(i, 0);
                emit dataChanged(start, end, roles);
            }
        }
    }
}
