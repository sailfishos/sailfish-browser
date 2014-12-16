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

    Q_PROPERTY(int activeTabIndex READ activeTabIndex NOTIFY activeTabIndexChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int nextTabId READ nextTabId NOTIFY nextTabIdChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(bool waitingForNewTab READ waitingForNewTab WRITE setWaitingForNewTab NOTIFY waitingForNewTabChanged FINAL)

public:
    DeclarativeTabModel(QObject *parent = 0);
    ~DeclarativeTabModel();
    
    enum TabRoles {
        ThumbPathRole = Qt::UserRole + 1,
        TitleRole,
        UrlRole,
        TabIdRole
    };

    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool activateTab(const QString &url);
    Q_INVOKABLE bool activateTab(int index, bool loadActiveTab = true);
    Q_INVOKABLE void closeActiveTab();
    Q_INVOKABLE void newTab(const QString &url, const QString &title, int parentId = 0);

    Q_INVOKABLE void dumpTabs() const;

    int activeTabIndex() const;
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
    void setUnloaded();

    bool waitingForNewTab() const;
    void setWaitingForNewTab(bool waiting);

    const QList<Tab>& tabs() const;
    const Tab& activeTab() const;

    bool contains(int tabId) const;

    void updateUrl(int tabId, bool activeTab, QString url, bool backForwardNavigation, bool initialLoad = false);
    void updateTitle(int tabId, bool activeTab, QString url, QString title);

public slots:
    // TODO: Move to be private
    void tabsAvailable(QList<Tab> tabs);
    void updateThumbnailPath(int tabId, QString path);

signals:
    void activeTabIndexChanged();
    void countChanged();
    void activeTabChanged(int oldTabId, int activeTabId, bool loadActiveTab = true);
    // TODO: Update test to use activeTabChanged instead. Currently this is here
    // only for testing purposes.
    void tabAdded(int tabId);
    void tabClosed(int tabId);
    void tabsCleared();
    void nextTabIdChanged();
    void loadedChanged();
    void waitingForNewTabChanged();
    void newTabRequested(QString url, QString title, int parentId = 0);

private slots:
    void tabChanged(const Tab &tab);
    void saveActiveTab() const;

private:
    void removeTab(int tabId, const QString &thumbnail, int index);
    int findTabIndex(int tabId) const;
    void updateActiveTab(const Tab &activeTab, bool loadActiveTab);
    void updateTabUrl(int tabId, bool activeTab, const QString &url, bool navigate);

    // This should be replaced by m_activeTabIndex
    Tab m_activeTab;
    QList<Tab> m_tabs;

    bool m_loaded;
    bool m_waitingForNewTab;
    int m_nextTabId;

    friend class tst_declarativetabmodel;
    friend class tst_webview;
};
#endif // DECLARATIVETABMODEL_H
