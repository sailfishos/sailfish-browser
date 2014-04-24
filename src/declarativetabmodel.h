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

#include "tab.h"

class DeclarativeTabModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int nextTabId READ nextTabId NOTIFY nextTabIdChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(bool browsing READ browsing WRITE setBrowsing NOTIFY browsingChanged FINAL)
    // TODO: Remove newTab related functions from WebContainer, WebPage, and TabModel pushed to C++ side.
    Q_PROPERTY(bool hasNewTabData READ hasNewTabData NOTIFY hasNewTabDataChanged FINAL)
    // TODO: Only needed by on_ReadyToLoadChanged handler of WebView
    Q_PROPERTY(QString newTabUrl READ newTabUrl NOTIFY newTabUrlChanged FINAL)

public:
    DeclarativeTabModel(QObject *parent = 0);
    
    enum TabRoles {
        ThumbPathRole = Qt::UserRole + 1,
        TitleRole,
        UrlRole,
        TabIdRole
    };

    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool activateTab(const QString &url);
    Q_INVOKABLE bool activateTab(int index);
    Q_INVOKABLE void closeActiveTab();

    Q_INVOKABLE void newTab(const QString &url, const QString &title);
    // This should be only for C++ side. But there is one depending move to QML side (see WebView::load())
    Q_INVOKABLE void newTabData(const QString &url, const QString &title, QObject *contentItem = 0, int parentId = 0);
    Q_INVOKABLE void resetNewTabData();

    Q_INVOKABLE void dumpTabs() const;

    int count() const;
    void addTab(const QString &url, const QString &title);
    bool activateTabById(int tabId);
    void removeTabById(int tabId, bool activeTab);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    // From QQmlParserStatus
    void classBegin();
    void componentComplete();

    int nextTabId() const;

    bool loaded() const;

    bool browsing() const;
    void setBrowsing(bool browsing);

    bool hasNewTabData() const;
    QString newTabUrl() const;

    int newTabParentId() const;

    bool backForwardNavigation() const;
    void setBackForwardNavigation(bool backForwardNavigation);

    QObject* newTabPreviousPage() const;
    const QList<Tab>& tabs() const;
    const Tab& activeTab() const;

    void updateUrl(int tabId, bool activeTab, QString url, bool initialLoad = false);
    void updateTitle(int tabId, bool activeTab, QString title);
    void updateThumbnailPath(int tabId, bool activeTab, QString path);

    static bool tabSort(const Tab &t1, const Tab &t2);

public slots:
    void tabsAvailable(QList<Tab> tabs);

signals:
    void countChanged();
    void activeTabChanged(int oldTabId, int activeTabId);
    void tabAdded(int tabId);
    void tabClosed(int tabId);
    void tabsCleared();
    void nextTabIdChanged();
    void loadedChanged();
    void browsingChanged();
    void hasNewTabDataChanged();
    void newTabUrlChanged();
    void newTabRequested(QString url, QString title);
    void updateActiveThumbnail();

private slots:
    void tabChanged(const Tab &tab);
    void updateTitle(int tabId, int linkId, QString url, QString title);

private:
    struct NewTabData {
        NewTabData(QString url, QString title, QObject *previousPage, int parentId)
            : url(url)
            , title(title)
            , previousPage(previousPage)
            , parentId(parentId)
        {}

        QString url;
        QString title;
        QObject *previousPage;
        int parentId;
    };

    void load();
    void removeTab(int tabId, const QString &thumbnail, int index = -1);
    int findTabIndex(int tabId) const;
    void saveTabOrder();
    void loadTabOrder();
    void updateActiveTab(const Tab &activeTab);
    void updateTabUrl(int tabId, bool activeTab, const QString &url, bool navigate);

    void updateNewTabData(NewTabData *newTabData);
    QString newTabTitle() const;

    Tab m_activeTab;
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
