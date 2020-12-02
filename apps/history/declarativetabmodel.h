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
#include <QPointer>
#include <QScopedPointer>

#include "tab.h"

class DeclarativeWebContainer;

class DeclarativeTabModel : public QAbstractListModel
{
    Q_OBJECT

protected:
    Q_PROPERTY(int activeTabIndex READ activeTabIndex NOTIFY activeTabIndexChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(bool waitingForNewTab READ waitingForNewTab WRITE setWaitingForNewTab NOTIFY waitingForNewTabChanged FINAL)

public:
    DeclarativeTabModel(int nextTabId, DeclarativeWebContainer *webContainer = 0);
    ~DeclarativeTabModel();

    enum TabRoles {
        ThumbPathRole = Qt::UserRole + 1,
        TitleRole,
        UrlRole,
        ActiveRole,
        TabIdRole
    };

    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool activateTab(const QString &url);
    Q_INVOKABLE void activateTab(int index);
    Q_INVOKABLE void closeActiveTab();
    Q_INVOKABLE int newTab(const QString &url, int parentId = 0);
    Q_INVOKABLE QString url(int tabId) const;

    Q_INVOKABLE void dumpTabs() const;

    int activeTabIndex() const;
    int activeTabId() const;
    int count() const;
    bool activateTabById(int tabId);
    void removeTabById(int tabId, bool activeTab);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    int nextTabId() const;

    bool loaded() const;
    void setUnloaded();

    bool waitingForNewTab() const;
    void setWaitingForNewTab(bool waiting);

    const QList<Tab>& tabs() const;
    const Tab& activeTab() const;

    bool contains(int tabId) const;

public slots:
    void updateThumbnailPath(int tabId, const QString &path);
    void onUrlChanged();
    void onTitleChanged();

signals:
    void activeTabIndexChanged();
    void countChanged();
    void activeTabChanged(int activeTabId);
    void tabAdded(int tabId);
    void tabClosed(int tabId);
    void loadedChanged();
    void waitingForNewTabChanged();
    void newTabRequested(const Tab& tab, int parentId = 0);

protected:
    void addTab(const QString &url, const QString &title, int index);
    void removeTab(int tabId, const QString &thumbnail, int index);
    int findTabIndex(int tabId) const;
    void updateActiveTab(const Tab &activeTab);
    void updateUrl(int tabId, const QString &url, bool initialLoad);

    virtual void createTab(const Tab &tab) = 0;
    virtual void updateTitle(int tabId, const QString &url, const QString &title) = 0;
    virtual void removeTab(int tabId) = 0;
    virtual void navigateTo(int tabId, const QString &url, const QString &title, const QString &path) = 0;
    virtual void updateThumbPath(int tabId, const QString &path) = 0;

    int nextActiveTabIndex(int index);

    // Used from the tab model unit tests only.
    void setWebContainer(DeclarativeWebContainer *webContainer);

    int m_activeTabId;
    QList<Tab> m_tabs;

    bool m_loaded;
    bool m_waitingForNewTab;
    int m_nextTabId;

    QPointer<DeclarativeWebContainer> m_webContainer;

    friend class tst_declarativehistorymodel;
    friend class tst_declarativetabmodel;
    friend class tst_webview;
    friend class tst_declarativewebcontainer;
    friend class tst_persistenttabmodel;
};
#endif // DECLARATIVETABMODEL_H
