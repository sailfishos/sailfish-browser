/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "webpages.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QMapIterator>
#include <QRectF>
#include <qqmlinfo.h>

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

#if DEBUG_LOGS
#include <QDebug>
#endif

WebPages::WebPages(QObject *parent)
    : QObject(parent)
    , m_activePage(0)
    , m_count(0)
{
}

WebPages::~WebPages()
{
    QMapIterator<int, WebPageEntry*> pages(m_activePages);
    while (pages.hasNext()) {
        pages.next();
        WebPageEntry *pageEntry = pages.value();
        delete pageEntry;
    }
}

void WebPages::initialize(DeclarativeWebContainer *webContainer, QQmlComponent *webPageComponent)
{
    if (!m_webContainer || !m_webPageComponent) {
        m_webContainer = webContainer;
        m_webPageComponent = webPageComponent;
    }
}

bool WebPages::initialized() const
{
    return m_webContainer && m_webPageComponent;
}

int WebPages::count() const
{
    return m_count;
}

WebPageActivationData WebPages::page(int tabId, int parentId)
{
    if (!m_webPageComponent) {
        qWarning() << "TabModel not initialized!";
        return WebPageActivationData(0, false);
    }

    if (m_activePage && m_activePage->webPage && m_activePage->webPage->tabId() == tabId) {
        m_activePage->webPage->resumeView();
        m_activePage->webPage->setVisible(true);
        return WebPageActivationData(m_activePage->webPage, false);
    }

#if DEBUG_LOGS
    qDebug() << "about to create a new tab or activate old:" << tabId;
#endif

    WebPageEntry *pageEntry = m_activePages.value(tabId, 0);
    bool resurrect = pageEntry && !pageEntry->webPage;
    if (!pageEntry || resurrect) {
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
                if (!pageEntry) {
                    pageEntry = new WebPageEntry(webPage, 0);
                } else {
                    pageEntry->webPage = webPage;
                }

                m_webPageComponent->completeCreate();
#if DEBUG_LOGS
                qDebug() << "New view id:" << webPage->uniqueID() << "parentId:" << webPage->parentId() << "tab id:" << webPage->tabId();
#endif
                m_activePages.insert(tabId, pageEntry);
                ++m_count;
                QQmlEngine::setObjectOwnership(webPage, QQmlEngine::CppOwnership);
            } else {
                qmlInfo(m_webContainer) << "webPage component must be a WebPage component";
            }
        } else {
            qmlInfo(m_webContainer) << "Creation of the web page failed. Error: " << m_webPageComponent->errorString();
            delete object;
            object = 0;
        }
    }

    updateActivePage(pageEntry, resurrect);
#if DEBUG_LOGS
    dumpPages();
#endif

    return WebPageActivationData(pageEntry->webPage, true);
}

void WebPages::release(int tabId, bool virtualize)
{
    WebPageEntry *pageEntry = m_activePages.value(tabId, 0);
#if DEBUG_LOGS
    qDebug() << "--- beginning: " << tabId << virtualize << pageEntry << (pageEntry ? pageEntry->webPage : 0);
    dumpPages();
#endif
    if (pageEntry) {
        --m_count;
        DeclarativeWebPage *activeWebPage = m_activePage && m_activePage->webPage ? m_activePage->webPage : 0;
        if (m_count == 0 || (activeWebPage && activeWebPage->tabId() == tabId)) {
            m_activePage = 0;
        }

        if (pageEntry->webPage) {
            if (pageEntry->webPage->viewReady()) {
                pageEntry->webPage->setParent(0);
                delete pageEntry->webPage;
            } else {
                connect(pageEntry->webPage, SIGNAL(viewReadyChanged()), pageEntry->webPage, SLOT(deleteLater()));
            }
        }

        pageEntry->webPage = 0;
        if (virtualize) {
            m_activePages.insert(tabId, pageEntry);
        } else {
            delete pageEntry;
            m_activePages.remove(tabId);
        }
        if (m_activePages.isEmpty() || m_count < 0) {
            m_count = 0;
            m_activePage = 0;
        }
    }

#if DEBUG_LOGS
    qDebug() << "--- end ---";
    dumpPages();
#endif
}

void WebPages::clear()
{
    QList<WebPageEntry *> pages = m_activePages.values();
    int count = pages.count();
    for (int i = 0; i < count; ++i) {
        WebPageEntry *pageEntry = pages.at(i);
        pageEntry->allowPageDelete = true;
        delete pageEntry;
    }
    m_activePages.clear();
    m_count = 0;
    m_activePage = 0;
}

int WebPages::parentTabId(int tabId) const
{
    WebPageEntry *pageEntry = m_activePages.value(tabId, 0);
    if (pageEntry && pageEntry->webPage) {
        int parentId = pageEntry->webPage->parentId();
        QMapIterator<int, WebPageEntry*> pages(m_activePages);
        while (pages.hasNext()) {
            pages.next();
            WebPageEntry *parentPageEntry = pages.value();
            if (parentPageEntry->webPage && (int)parentPageEntry->webPage->uniqueID() == parentId) {
                return parentPageEntry->webPage->tabId();
            }
        }
    }
    return 0;
}

void WebPages::updateActivePage(WebPageEntry *webPageEntry, bool resurrect)
{
    DeclarativeWebPage * activeWebPage = 0;
    if (m_activePage && (activeWebPage = m_activePage->webPage)) {
        m_activePage->cssContentRect = new QRectF(activeWebPage->contentRect());
        activeWebPage->setVisible(false);

        // Allow suspending only the current active page if it is not the creator (parent).
        if (webPageEntry->webPage->parentId() != (int)activeWebPage->uniqueID()) {
             if (activeWebPage->loading()) {
                 activeWebPage->stop();
             }
             activeWebPage->suspendView();
        }
    }

    m_activePage = webPageEntry;
    activeWebPage = m_activePage->webPage;
    if (resurrect && activeWebPage) {
        // Copy rect value
        activeWebPage->setResurrectedContentRect(*m_activePage->cssContentRect);
        delete m_activePage->cssContentRect;
        m_activePage->cssContentRect = 0;
    }

    if (activeWebPage) {
        activeWebPage->resumeView();
        activeWebPage->setVisible(true);
    }
}

void WebPages::dumpPages() const
{
    qDebug() << "---- start ----";
    QMapIterator<int, WebPageEntry*> pages(m_activePages);
    while (pages.hasNext()) {
        pages.next();
        WebPageEntry *pageEntry = pages.value();
        qDebug() << "tabId: " << pages.key() << "page: " << pageEntry->webPage
                 << "title:" << (pageEntry->webPage ? pageEntry->webPage->title() : "VIEW NOT ALIVE!")
                 << "cssContentRect:" << pageEntry->cssContentRect;
    }
    qDebug() << "---- end ------";
}

QList<int> WebPages::liveTabs()
{
    QList<WebPageEntry *> pages = m_activePages.values();
    QList<int> tabIds;
    int count = pages.count();
    for (int i = 0; i < count; ++i) {
        WebPageEntry *pageEntry = pages.at(i);
        if (pageEntry->webPage) {
            tabIds << pageEntry->webPage->tabId();
        }
    }
    return tabIds;
}

QList<int> WebPages::zombifiedTabs()
{
    QMapIterator<int, WebPageEntry*> pages(m_activePages);
    QList<int> tabIds;

    while (pages.hasNext()) {
        pages.next();
        if (!pages.value()->webPage) {
            tabIds << pages.key();
        }
    }
    return tabIds;
}

WebPages::WebPageEntry::WebPageEntry(DeclarativeWebPage *webPage, QRectF *cssContentRect)
    : webPage(webPage)
    , cssContentRect(cssContentRect)
    , allowPageDelete(false)
{
}

WebPages::WebPageEntry::~WebPageEntry()
{
    if (cssContentRect) {
        delete cssContentRect;
    }

    if (webPage && (webPage->viewReady() || allowPageDelete)) {
        webPage->setParent(0);
        delete webPage;
    }

    cssContentRect = 0;
    webPage = 0;
}
