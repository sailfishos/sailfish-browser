/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "tabcache.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QMapIterator>
#include <QRectF>
#include <qqmlinfo.h>

#ifdef DEBUG_LOGS
#include <QDebug>
#endif

TabCache::TabCache(QObject *parent)
    : QObject(parent)
    , m_activeTab(0)
    , m_count(0)
{
}

TabCache::~TabCache()
{
    QMapIterator<int, TabEntry*> tabs(m_activeTabs);
    while (tabs.hasNext()) {
        tabs.next();
        TabEntry *tabEntry = tabs.value();
        delete tabEntry;
    }
}

void TabCache::initialize(DeclarativeWebContainer *webContainer, QQmlComponent *webPageComponent)
{
    if (!m_webContainer || !m_webPageComponent) {
        m_webContainer = webContainer;
        m_webPageComponent = webPageComponent;
    }
}

bool TabCache::initialized() const
{
    return m_webContainer && m_webPageComponent;
}

int TabCache::count() const
{
    return m_count;
}

TabActivationData TabCache::tab(int tabId, int parentId)
{
    if (!m_webPageComponent) {
        qWarning() << "TabModel not initialized!";
        return TabActivationData(0, false);
    }

    if (m_activeTab && m_activeTab->webPage && m_activeTab->webPage->tabId() == tabId) {
        m_activeTab->webPage->resumeView();
        m_activeTab->webPage->setVisible(true);
        return TabActivationData(m_activeTab->webPage, false);
    }

#ifdef DEBUG_LOGS
    qDebug() << "about to create a new tab or activate old:" << tabId;
#endif

    TabEntry *tabEntry = m_activeTabs.value(tabId, 0);
    bool resurrect = tabEntry && !tabEntry->webPage;
    if (!tabEntry || resurrect) {
        QQmlContext *creationContext = m_webPageComponent->creationContext();
        QQmlContext *context = new QQmlContext(creationContext ? creationContext : QQmlEngine::contextForObject(m_webContainer));
        QObject *object = m_webPageComponent->beginCreate(context);
        if (object) {
            context->setParent(object);
            object->setParent(m_webContainer);
            DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(object);
            if (webPage) {
                webPage->setParentItem(m_webContainer);
                webPage->setParentID(parentId);
                webPage->setTabId(tabId);
                webPage->setContainer(m_webContainer);
                if (!tabEntry) {
                    tabEntry = new TabEntry(webPage, 0);
                } else {
                    tabEntry->webPage = webPage;
                }

                m_webPageComponent->completeCreate();
#ifdef DEBUG_LOGS
                qDebug() << "New view id:" << webPage->uniqueID() << "parentId:" << webPage->parentId() << "tab id:" << webPage->tabId();
#endif
                m_activeTabs.insert(tabId, tabEntry);
                ++m_count;
            } else {
                qmlInfo(m_webContainer) << "webPage component must be a WebPage component";
            }
        } else {
            qmlInfo(m_webContainer) << "Creation of the web page failed. Error: " << m_webPageComponent->errorString();
            delete object;
            object = 0;
        }
    }

    updateActiveTab(tabEntry, resurrect);
#ifdef DEBUG_LOGS
    dumpTabs();
#endif

    return TabActivationData(tabEntry->webPage, true);
}

void TabCache::release(int tabId, bool virtualize)
{
    TabEntry *tabEntry = m_activeTabs.value(tabId, 0);
#ifdef DEBUG_LOGS
    qDebug() << "--- beginning: " << tabId << (tabEntry ? tabEntry->webPage : 0);
    dumpTabs();
#endif
    if (tabEntry) {
        --m_count;
        DeclarativeWebPage *activeWebPage = m_activeTab && m_activeTab->webPage ? m_activeTab->webPage : 0;
        if (m_count == 0 || (activeWebPage && activeWebPage->tabId() == tabId)) {
            m_activeTab = 0;
        }

        delete tabEntry->webPage;
        tabEntry->webPage = 0;
        if (virtualize) {
            m_activeTabs.insert(tabId, tabEntry);
        } else {
            delete tabEntry;
            m_activeTabs.remove(tabId);
        }
    }

#ifdef DEBUG_LOGS
    qDebug() << "--- end ---";
    dumpTabs();
#endif
}

int TabCache::parentTabId(int tabId) const
{
    TabEntry *tabEntry = m_activeTabs.value(tabId, 0);
    if (tabEntry && tabEntry->webPage) {
        int parentId = tabEntry->webPage->parentId();
        QMapIterator<int, TabEntry*> tabs(m_activeTabs);
        while (tabs.hasNext()) {
            tabs.next();
            TabEntry *parentTabEntry = tabs.value();
            if (parentTabEntry->webPage && (int)parentTabEntry->webPage->uniqueID() == parentId) {
                return parentTabEntry->webPage->tabId();
            }
        }
    }
    return 0;
}

void TabCache::updateActiveTab(TabEntry *tab, bool resurrect)
{
    DeclarativeWebPage * activeWebPage = 0;
    if (m_activeTab && (activeWebPage = m_activeTab->webPage)) {
        m_activeTab->cssContentRect = new QRectF(activeWebPage->contentRect());
        activeWebPage->setVisible(false);

        // Allow subpending only current active is not creator (parent).
        if (tab->webPage->parentId() != (int)activeWebPage->uniqueID()) {
             if (activeWebPage->loading()) {
                 activeWebPage->stop();
             }
             activeWebPage->suspendView();
        }
    }

    m_activeTab = tab;
    activeWebPage = m_activeTab->webPage;
    if (resurrect && activeWebPage) {
        // Copy rect value
        activeWebPage->setResurrectedContentRect(*m_activeTab->cssContentRect);
        delete m_activeTab->cssContentRect;
        m_activeTab->cssContentRect = 0;
    }

    if (activeWebPage) {
        activeWebPage->resumeView();
        activeWebPage->setVisible(true);
    }
}

void TabCache::dumpTabs() const
{
    qDebug() << "---- start ----";
    QMapIterator<int, TabEntry*> tabs(m_activeTabs);
    while (tabs.hasNext()) {
        tabs.next();
        TabEntry *tabEntry = tabs.value();
        qDebug() << "tabId: " << tabs.key() << "page: " << tabEntry->webPage
                 << "title:" << (tabEntry->webPage ? tabEntry->webPage->title() : "VIEW NOT ALIVE!")
                 << "cssContentRect:" << tabEntry->cssContentRect;
    }
    qDebug() << "---- end ------";
}


TabCache::TabEntry::TabEntry(DeclarativeWebPage *webPage, QRectF *cssContentRect)
    : webPage(webPage)
    , cssContentRect(cssContentRect)
{
}

TabCache::TabEntry::~TabEntry()
{
    if (cssContentRect) {
        delete cssContentRect;
    }

    if (webPage) {
        webPage->setParent(0);
        delete webPage;
    }

    cssContentRect = 0;
    webPage = 0;
}
