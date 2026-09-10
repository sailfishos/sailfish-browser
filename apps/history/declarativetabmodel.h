/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVETABMODEL_H
#define DECLARATIVETABMODEL_H

#include <QAbstractListModel>

#include "tab.h"

class DeclarativeWebContainer;

class DeclarativeTabModel : public QAbstractListModel
{
    Q_OBJECT

protected:
    Q_PROPERTY(int activeTabIndex READ activeTabIndex NOTIFY activeTabIndexChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)

public:
    DeclarativeTabModel(int nextTabId, DeclarativeWebContainer *webContainer = 0);
    ~DeclarativeTabModel();

    enum TabRoles {
        ThumbPathRole = Qt::UserRole + 1,
        TitleRole,
        UrlRole,
        ActiveRole,
        TabIdRole,
        DesktopModeRole,
        HiddenRole,
    };

    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool activateTab(const QString &url, bool reload = false);
    Q_INVOKABLE void activateTab(int index, bool reload = false);
    Q_INVOKABLE void closeActiveTab();
    Q_INVOKABLE int newTab(const QString &url, bool fromExternal);
    Q_INVOKABLE QString url(int tabId) const;

    Q_INVOKABLE void dumpTabs() const;

    int activeTabIndex() const;
    int activeTabId() const;
    int count() const;
    bool activateTabById(int tabId);
    void removeTabById(int tabId, bool activeTab);
    bool requestRuntimeTabNavigation(int tabId, const QString &url,
                                     bool fromExternal = false);
    Q_INVOKABLE bool runtimeNavigateTab(const QString &persistentId,
                                        const QString &url,
                                        bool fromExternal = false);
    // C++ only: parentId and browsingContext better not to leak to QML side.
    int newTab(const QString &url, int parentId, uintptr_t browsingContext, bool hidden, bool fromExternal);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int nextTabId() const;

    bool loaded() const;

    const QList<Tab>& tabs() const;
    const Tab& activeTab() const;
    Tab *getTab(int tabId);

    bool contains(int tabId) const;

public slots:
    void updateThumbnailPath(int tabId, const QString &path);

signals:
    void activeTabIndexChanged();
    void countChanged();
    void tabClosed(int tabId);
    void loadedChanged();
    void runtimeNewTabRequested(const QString &url, const QString &persistentId,
                                bool fromExternal);
    void runtimeTabActivationRequested(const QString &persistentId, bool reload);
    void runtimeTabNavigationRequested(const QString &persistentId,
                                       const QString &url, bool fromExternal);
    void runtimeTabCloseRequested(const QString &persistentId);
    void runtimeTabsClearRequested();

protected:
    int findTabIndex(int tabId) const;

    virtual void createTab(const Tab &tab) = 0;
    virtual void removeTab(int tabId) = 0;
    virtual void updateThumbPath(int tabId, const QString &path) = 0;

    bool matches(const QUrl &inputUrl, QString urlStr) const;

    int m_activeTabId;
    QList<Tab> m_tabs;

    bool m_loaded;
    int m_nextTabId;

    bool m_unittestMode;

    friend class tst_declarativehistorymodel;
    friend class tst_declarativetabmodel;
    friend class tst_webview;
    friend class tst_declarativewebcontainer;
    friend class tst_persistenttabmodel;
};
#endif // DECLARATIVETABMODEL_H
