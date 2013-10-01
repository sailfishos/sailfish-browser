/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#include "declarativetabmodel.h"

#include "dbmanager.h"

DeclarativeTabModel::DeclarativeTabModel(QObject *parent) :
    QAbstractListModel(parent), m_currentTabIndex(-1)
{
    connect(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)),
            this, SLOT(tabsAvailable(QList<Tab>)));

    connect(DBManager::instance(), SIGNAL(tabChanged(Tab)),
            this, SLOT(tabChanged(Tab)));

    connect(DBManager::instance(), SIGNAL(thumbPathChanged(QString,QString)),
            this, SLOT(updateThumbPath(QString,QString)));
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

void DeclarativeTabModel::addTab(const QString& url, bool foreground) {
    int tabId = DBManager::instance()->createTab();
    Tab tab(tabId, Link(0, url, "", ""), 0, 0);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_tabs.append(tab);
    endInsertRows();
    emit countChanged();
    if (foreground) {
        setCurrentTabIndex(m_tabs.count() - 1);
    }
}

int DeclarativeTabModel::currentTabId() const
{
    if (!m_tabs.isEmpty() && m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.count()) {
        return m_tabs.at(m_currentTabIndex).tabId();
    }
    return 0;
}

void DeclarativeTabModel::remove(const int index) {
    if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
        beginRemoveRows(QModelIndex(), index, index);
        int tabId = m_tabs.at(index).tabId();
        m_tabs.removeAt(index);
        endRemoveRows();
        DBManager::instance()->removeTab(tabId);
        emit countChanged();
        int newIndex = -1;
        if (!m_tabs.isEmpty() && index >= 0 && index < m_tabs.count()) {
            newIndex = index;
        } else if (!m_tabs.isEmpty()) {
            newIndex = m_tabs.count() - 1;
        }
        if (newIndex != m_currentTabIndex) {
            m_currentTabIndex = newIndex;
            DBManager::instance()->saveSetting("currentTab", QString("%1").arg(m_currentTabIndex));
        }
        emit currentTabChanged();
    }
}

void DeclarativeTabModel::clear()
{
    if (m_tabs.count() == 0)
        return;

    beginRemoveRows(QModelIndex(), 0, m_tabs.count() - 1);
    m_tabs.clear();
    endRemoveRows();
    DBManager::instance()->removeAllTabs();
    emit countChanged();
    m_currentTabIndex = -1;
    DBManager::instance()->saveSetting("currentTab", QString("%1").arg(m_currentTabIndex));
}


bool DeclarativeTabModel::activateTab(const QString& url)
{
    if (m_currentTabIndex >= 0 && m_tabs.at(m_currentTabIndex).currentLink().url() == url) {
        return true;
    }
    for (int i = 0; i < m_tabs.size(); i++) {
        if (m_tabs.at(i).currentLink().url() == url) {
            setCurrentTabIndex(i);
            return true;
        }
    }
    return false;
}

int DeclarativeTabModel::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return m_tabs.count();
}

QVariant DeclarativeTabModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() > m_tabs.count())
        return QVariant();

    const Tab tab = m_tabs[index.row()];
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

void DeclarativeTabModel::setCurrentTabIndex(const int index)
{
    if (index >= 0 && index < m_tabs.count() && index != m_currentTabIndex) {
        m_currentTabIndex = index;
        emit currentTabChanged();
        DBManager::instance()->saveSetting("currentTab", QString("%1").arg(m_currentTabIndex));
    }
}

int DeclarativeTabModel::currentTabIndex() const
{
    return m_currentTabIndex;
}

void DeclarativeTabModel::classBegin()
{
    DBManager::instance()->getAllTabs();
}

void DeclarativeTabModel::tabsAvailable(QList<Tab> tabs)
{
    beginResetModel();
    m_tabs.clear();
    m_tabs = tabs;
    endResetModel();

    if (m_tabs.count() > 0) {
        emit countChanged();
    }

    QString currentTabIndexSetting = DBManager::instance()->getSetting("currentTab");
    bool ok;
    int currentTabIndex = currentTabIndexSetting.toInt(&ok);
    if (ok && currentTabIndex >= 0 && currentTabIndex < m_tabs.count()) {
        m_currentTabIndex = currentTabIndex;
        emit currentTabChanged();
    } else {
        if (m_tabs.count() > 1) {
            setCurrentTabIndex(0);
        }
    }
}

void DeclarativeTabModel::tabChanged(Tab tab)
{
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

void DeclarativeTabModel::updateThumbPath(QString url, QString path)
{
    QVector<int> roles;
    roles << ThumbPathRole;
    for (int i = 0; i < m_tabs.count(); i++) {
        if (m_tabs.at(i).currentLink().url() == url) {
            if (m_tabs.at(i).currentLink().thumbPath() != path) {
                Link tmp = m_tabs[i].currentLink();
                tmp.setThumbPath(path);
                m_tabs[i].setCurrentLink(tmp);
                QModelIndex start = index(i, 0);
                QModelIndex end = index(i, 0);
                emit dataChanged(start, end, roles);
            }
        }
    }
}

void DeclarativeTabModel::updateTitle(QString url, QString title)
{
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
