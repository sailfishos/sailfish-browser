/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TABCACHE_H
#define TABCACHE_H

#include <QObject>
#include <QMap>
#include <QPointer>

class QQmlComponent;
class DeclarativeWebContainer;
class DeclarativeWebPage;

struct TabActivationData {
    TabActivationData(DeclarativeWebPage *webPage, bool activated)
        : webPage(webPage)
        , activated(activated)
    {}

    DeclarativeWebPage *webPage;
    bool activated;
};

class TabCache : public QObject
{
    Q_OBJECT

public:
    explicit TabCache(QObject *parent = 0);
    ~TabCache();

    void initialize(DeclarativeWebContainer *webContainer, QQmlComponent *webPageComponent);
    bool initialized() const;
    int count() const;

    TabActivationData tab(int tabId, int parentId = 0);
    void release(int tabId, bool virtualize = false);
    int parentTabId(int tabId) const;

private:
    struct TabEntry {
        TabEntry(DeclarativeWebPage *webPage, QRectF *cssContentRect);
        ~TabEntry();

        DeclarativeWebPage *webPage;
        QRectF *cssContentRect;
    };

    void updateActiveTab(TabEntry *tab, bool resurrect);
    void dumpTabs() const;

    QPointer<DeclarativeWebContainer> m_webContainer;
    QPointer<QQmlComponent> m_webPageComponent;
    // Contains both virtual and real tabs
    QMap<int, TabEntry*> m_activeTabs;
    TabEntry *m_activeTab;
    int m_count;
};

#endif
