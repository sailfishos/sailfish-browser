/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVETABMODEL_H
#define DECLARATIVETABMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <QPointer>

#include "tab.h"

class DeclarativeTab;

class DeclarativeTabModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int currentTabId READ currentTabId NOTIFY currentTabIdChanged FINAL)
    Q_PROPERTY(int nextTabId READ nextTabId NOTIFY nextTabIdChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(bool browsing READ browsing WRITE setBrowsing NOTIFY browsingChanged FINAL)

public:
    DeclarativeTabModel(QObject *parent = 0);
    
    enum TabRoles {
        ThumbPathRole = Qt::UserRole + 1,
        TitleRole,
        UrlRole,
        TabIdRole
    };

    Q_INVOKABLE void addTab(const QString &url, const QString &title);
    Q_INVOKABLE void remove(const int index);
    Q_INVOKABLE void removeTabById(const int &tabId);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool activateTab(const QString &url);
    Q_INVOKABLE bool activateTab(const int &index);
    Q_INVOKABLE void activateTabById(const int &tabId);
    Q_INVOKABLE void closeActiveTab();
    Q_INVOKABLE int lastTabId() const;

    Q_INVOKABLE void dumpTabs() const;

    int count() const;

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    // From QQmlParserStatus
    void classBegin();
    void componentComplete();

    int currentTabId() const;
    int nextTabId() const;

    void setCurrentTab(DeclarativeTab *currentTab);

    bool loaded() const;

    bool browsing() const;
    void setBrowsing(bool browsing);

    bool backForwardNavigation() const;
    void setBackForwardNavigation(bool backForwardNavigation);

    static bool tabSort(const Tab &t1, const Tab &t2);

public slots:
    void tabsAvailable(QList<Tab> tabs);
    void updateUrl(int tabId, QString url);
    void updateTitle(int tabId, QString title);

signals:
    void countChanged();
    void activeTabChanged(int tabId);
    void tabAdded(int tabId);
    void tabClosed(int tabId);
    void currentTabChanged();
    void currentTabIdChanged();
    void nextTabIdChanged();
    void loadedChanged();
    void browsingChanged();

    void _activeTabInvalidated();
    void _activeTabChanged(const Tab &tab);

private slots:
    void updateThumbPath(QString url, QString path, int tabId);
    void tabChanged(const Tab &tab);

private:
    void load();
    void removeTab(int tabId, const QString &thumbnail, int index = -1);
    int findTabIndex(int tabId, bool &activeTab) const;
    void saveTabOrder();
    int loadTabOrder();
    void updateActiveTab(const Tab &newActiveTab);
    void updateTabUrl(int tabId, const QString &url, bool navigate);

    QPointer<DeclarativeTab> m_currentTab;
    QList<Tab> m_tabs;
    bool m_loaded;
    bool m_browsing;
    int m_nextTabId;
    bool m_backForwardNavigation;

    friend class tst_declarativetabmodel;
};
#endif // DECLARATIVETABMODEL_H
