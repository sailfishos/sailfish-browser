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
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "linkvalidator.h"
#include "downloadmanager.h"

#include <QFile>
#include <QDebug>
#include <QStringList>
#include <quickmozview.h>

static QList<int> s_tabOrder;

DeclarativeTabModel::DeclarativeTabModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_currentTab(0)
    , m_loaded(false)
    , m_browsing(false)
    , m_maxLiveTabCount(5)
    , m_nextTabId(DBManager::instance()->getMaxTabId() + 1)
    , m_backForwardNavigation(false)
    , m_webPageComponent(0)
    , m_webView(0)
{
    m_tabCache.reset(new TabCache(this));
    connect(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)),
            this, SLOT(tabsAvailable(QList<Tab>)));
    connect(DBManager::instance(), SIGNAL(tabChanged(Tab)),
            this, SLOT(tabChanged(Tab)));
    connect(DBManager::instance(), SIGNAL(thumbPathChanged(QString,QString,int)),
            this, SLOT(updateThumbPath(QString,QString,int)));
    connect(DownloadManager::instance(), SIGNAL(downloadStarted()), this, SLOT(onDownloadStarted()));
    connect(this, SIGNAL(activeTabChanged(int)), this, SLOT(onActiveTabChanged(int)));
    connect(this, SIGNAL(maxLiveTabCountChanged()), this, SLOT(manageMaxTabCount()));
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
    manageMaxTabCount();

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

void DeclarativeTabModel::remove(int index) {
    if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
        beginRemoveRows(QModelIndex(), index, index);
        removeTab(m_tabs.at(index).tabId(), m_tabs.at(index).currentLink().thumbPath(), index);
        endRemoveRows();
        saveTabOrder();
    }
}

void DeclarativeTabModel::removeTabById(int tabId)
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

bool DeclarativeTabModel::activateTabById(int tabId)
{
    bool isActiveTab = false;
    int index = findTabIndex(tabId, isActiveTab);

    if (!isActiveTab && index >= 0) {
        return activateTab(index);
    }
    return false;
}

bool DeclarativeTabModel::activatePage(int tabId, bool force)
{
    m_tabCache->initialize(m_webView.data(), m_webPageComponent.data());
    if ((m_loaded || force) && tabId > 0 && m_tabCache->initialized()) {
        TabActivationData activationData = m_tabCache->tab(tabId, newTabParentId());
        m_webView->setWebPage(activationData.webPage);
        m_webView->webPage()->setChrome(true);
        m_webView->setLoadProgress(m_webView->webPage()->loadProgress());
        connect(m_webView->webPage(), SIGNAL(windowCloseRequested()), this, SLOT(closeWindow()), Qt::UniqueConnection);
        return activationData.activated;
    }
    return false;
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

void DeclarativeTabModel::releaseTab(int tabId, bool virtualize)
{
    if (m_tabCache && m_webView) {
        m_tabCache->release(tabId, virtualize);
        if (count() == 0) {
            m_webView->setWebPage(0);
        }
        resetNewTabData();
    }
}

void DeclarativeTabModel::newTab(const QString &url, const QString &title, int parentId)
{
    newTabData(url, title, m_webView ? m_webView->webPage() : 0, parentId);
    if (m_webView) {
        emit triggerLoad(url, title);
    }
}

void DeclarativeTabModel::newTabData(const QString &url, const QString &title, DeclarativeWebPage *contentItem, int parentId)
{
    bool urlChanged = newTabUrl() != url;
    bool titleChanged = newTabTitle() != title;

    updateNewTabData(new NewTabData(url, title, contentItem, parentId), urlChanged, titleChanged);
}

void DeclarativeTabModel::resetNewTabData()
{
    bool urlChanged = !newTabUrl().isEmpty();
    bool titleChanged = !newTabTitle().isEmpty();

    updateNewTabData(0, urlChanged, titleChanged);
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
        m_currentTab = currentTab;
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

DeclarativeWebPage *DeclarativeTabModel::newTabPreviousPage() const
{
    return hasNewTabData() ? m_newTabData->previousPage : 0;
}

int DeclarativeTabModel::newTabParentId() const
{
    return hasNewTabData() ? m_newTabData->parentId : 0;
}

int DeclarativeTabModel::parentTabId(int tabId) const
{
    if (m_tabCache) {
        return m_tabCache->parentTabId(tabId);
    }
    return 0;
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

        // Load placeholder for the case where no tabs exist. If a tab exists,
        // it gets initialized by onActiveTabChanged.
        if (m_webView && !m_webView->webPage()) {
            activatePage(nextTabId(), true);
        }

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

void DeclarativeTabModel::onActiveTabChanged(int tabId)
{
    if (!m_webView) {
        qWarning() << "No WebView available! This should not happen in normal circumstances, only with unit tests";
        return;
    }

    if (hasNewTabData()) {
        emit m_webView->currentTabChanged();
        return;
    }

    DeclarativeWebPage *currentPage = m_webView->webPage() ? m_webView->webPage() : 0;
    if (activatePage(tabId, true) && m_currentTab && m_currentTab->valid() && m_webView->readyToLoad()
            && (currentPage && (currentPage->tabId() != tabId || currentPage->url().toString() != m_currentTab->url()))) {
        emit triggerLoad(m_currentTab->url(), m_currentTab->title());
    }

    emit m_webView->currentTabChanged();
    manageMaxTabCount();
}

void DeclarativeTabModel::onDownloadStarted()
{
    // This is not 100% solid. A new tab is created for every incoming
    // url. In slow network connectivity one can close previous tab or
    // create a new tab before downloadStarted is emitted
    // by DownloadManager. To get this to the 100%, we would need to
    // pass windowId of the active window when download is started and close
    // the passed windowId instead.
    if (hasNewTabData() && m_webView && m_webView->webPage() && count() > 0) {
        DeclarativeWebPage *previousWebPage = newTabPreviousPage();
        releaseTab(m_webView->webPage()->tabId());
        if (previousWebPage) {
            activatePage(previousWebPage->tabId());
        } else if (count() == 0) {
            // Download doesn't add tab to model. Mimic
            // model change in case tabs count goes to zero.
            emit countChanged();
        }
    }
}

void DeclarativeTabModel::closeWindow()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage) {
        int parentPageTabId = parentTabId(webPage->tabId());
        // Closing only allowed if window was created by script
        if (parentPageTabId > 0) {
            activateTabById(parentPageTabId);
            removeTabById(webPage->tabId());
        }
    }
}

void DeclarativeTabModel::updateUrl(int tabId, QString url)
{
    if (m_currentTab && m_backForwardNavigation && m_currentTab->tabId() == tabId)
    {
        updateTabUrl(tabId, url, false);
    } else {
        if (!hasNewTabData() && m_currentTab) {
            updateTabUrl(tabId, url, true);
        } else {
            addTab(url, newTabTitle());
        }
    }
    resetNewTabData();
}

void DeclarativeTabModel::updateTitle(int tabId, QString title)
{
    bool isActiveTab = false;
    int tabIndex = findTabIndex(tabId, isActiveTab);

    bool updateDb = false;
    int linkId = -1;
    if (isActiveTab && m_currentTab->title() != title) {
        linkId = m_currentTab->linkId();
        m_currentTab->updateTabData(title);
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
    releaseTab(tabId);
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
        m_currentTab->updateTabData(url, "", "");
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

void DeclarativeTabModel::updateNewTabData(NewTabData *newTabData, bool urlChanged, bool titleChanged)
{
    bool hadNewTabData = hasNewTabData();
    m_newTabData.reset(newTabData);

    if (urlChanged) {
        emit newTabUrlChanged();
    }

    if (titleChanged) {
        emit newTabTitleChanged();
    }

    if (hadNewTabData != hasNewTabData()) {
        emit hasNewTabDataChanged();
    }
}

void DeclarativeTabModel::manageMaxTabCount()
{
    // Minimum is 1 tab.
    if (m_maxLiveTabCount < 1) {
        return;
    }

    // ActiveTab + m_maxLiveTabCount -1 == m_maxLiveTabCount
    for (int i = m_maxLiveTabCount - 1; i < m_tabs.count() && m_tabCache && m_tabCache->count() > m_maxLiveTabCount; ++i) {
        releaseTab(m_tabs.at(i).tabId(), true);
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
