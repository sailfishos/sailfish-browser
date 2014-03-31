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
#include <QScopedPointer>
#include <QQuickItem>
#include <QQmlComponent>

#include "tab.h"
#include "declarativewebcontainer.h"

class DeclarativeTab;
class DeclarativeWebPage;

class DeclarativeTabModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int currentTabId READ currentTabId NOTIFY currentTabIdChanged FINAL)
    Q_PROPERTY(int nextTabId READ nextTabId NOTIFY nextTabIdChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(bool browsing READ browsing WRITE setBrowsing NOTIFY browsingChanged FINAL)
    // TODO: Remove newTab related functions from WebContainer, WebPage, and TabModel pushed to C++ side.
    Q_PROPERTY(bool hasNewTabData READ hasNewTabData NOTIFY hasNewTabDataChanged FINAL)
    // TODO: Only needed by on_ReadyToLoadChanged handler of WebView
    Q_PROPERTY(QString newTabUrl READ newTabUrl NOTIFY newTabUrlChanged FINAL)
    // TODO: Only needed by WebPage::loadTab
    Q_PROPERTY(QString newTabTitle READ newTabTitle NOTIFY newTabTitleChanged FINAL)

public:
    DeclarativeTabModel(QObject *parent = 0);
    
    enum TabRoles {
        ThumbPathRole = Qt::UserRole + 1,
        TitleRole,
        UrlRole,
        TabIdRole
    };

    Q_INVOKABLE void addTab(const QString &url, const QString &title);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool activateTab(const QString &url);
    Q_INVOKABLE bool activateTab(int index);
    Q_INVOKABLE void closeActiveTab();

    Q_INVOKABLE void newTab(const QString &url, const QString &title, int parentId = 0);
    Q_INVOKABLE void newTabData(const QString &url, const QString &title, DeclarativeWebPage *contentItem = 0, int parentId = 0);
    Q_INVOKABLE void resetNewTabData();

    Q_INVOKABLE void dumpTabs() const;

    int count() const;
    bool activateTabById(int tabId);
    void removeTabById(int tabId);

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

    bool hasNewTabData() const;
    QString newTabUrl() const;
    QString newTabTitle() const;

    int newTabParentId() const;

    bool backForwardNavigation() const;
    void setBackForwardNavigation(bool backForwardNavigation);

    DeclarativeWebPage* newTabPreviousPage() const;
    const QList<Tab>& tabs() const;

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
    void hasNewTabDataChanged();
    void newTabUrlChanged();
    void newTabTitleChanged();
    void webViewChanged();
    void newTabRequested(QString url, QString title, int parentId);

    void _activeTabInvalidated();
    void _activeTabChanged(const Tab &tab);

private slots:
    void updateThumbPath(QString url, QString path, int tabId);
    void tabChanged(const Tab &tab);

private:
    struct NewTabData {
        NewTabData(QString url, QString title, DeclarativeWebPage *previousPage, int parentId)
            : url(url)
            , title(title)
            , previousPage(previousPage)
            , parentId(parentId)
        {}

        QString url;
        QString title;
        DeclarativeWebPage *previousPage;
        int parentId;
    };

    void load();
    void removeTab(int tabId, const QString &thumbnail, int index = -1);
    int findTabIndex(int tabId, bool &activeTab) const;
    void saveTabOrder();
    int loadTabOrder();
    void updateActiveTab(const Tab &newActiveTab);
    void updateTabUrl(int tabId, const QString &url, bool navigate);
    void updateNewTabData(NewTabData *newTabData, bool urlChanged, bool titleChanged);

    QPointer<DeclarativeTab> m_currentTab;
    QList<Tab> m_tabs;
    bool m_loaded;
    bool m_browsing;
    int m_nextTabId;
    bool m_backForwardNavigation;

    QScopedPointer<NewTabData> m_newTabData;

    friend class tst_declarativetabmodel;
    friend class tst_webview;
};
#endif // DECLARATIVETABMODEL_H
