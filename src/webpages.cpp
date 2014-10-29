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
{
}

WebPages::~WebPages()
{
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
    return m_activePages.count();
}

bool WebPages::setMaxLivePages(int count)
{
    return m_activePages.setMaxLivePages(count);
}

int WebPages::maxLivePages() const
{
    return m_activePages.maxLivePages();
}

WebPageActivationData WebPages::page(int tabId, int parentId)
{
    if (!m_webPageComponent) {
        qWarning() << "TabModel not initialized!";
        return WebPageActivationData(0, false);
    }

    if (m_activePages.active(tabId)) {
        DeclarativeWebPage *activePage = m_activePages.activeWebPage();
        activePage->resumeView();
        activePage->setVisible(true);
        return WebPageActivationData(activePage, false);
    }

#if DEBUG_LOGS
    qDebug() << "about to create a new tab or activate old:" << tabId;
#endif

    DeclarativeWebPage *webPage = 0;
    DeclarativeWebPage *oldActiveWebPage = m_activePages.activeWebPage();
    if (!m_activePages.alive(tabId)) {
        QQmlContext *creationContext = m_webPageComponent->creationContext();
        QQmlContext *context = new QQmlContext(creationContext ? creationContext : QQmlEngine::contextForObject(m_webContainer));
        QObject *object = m_webPageComponent->beginCreate(context);
        if (object) {
            context->setParent(object);
            object->setParent(m_webContainer);
            webPage = qobject_cast<DeclarativeWebPage *>(object);
            if (webPage) {
                webPage->setParentItem(m_webContainer);
                webPage->setParentID(parentId);
                webPage->setTabId(tabId);
                webPage->setContainer(m_webContainer);
                m_webPageComponent->completeCreate();
#if DEBUG_LOGS
                qDebug() << "New view id:" << webPage->uniqueID() << "parentId:" << webPage->parentId() << "tab id:" << webPage->tabId();
#endif
                m_activePages.prepend(tabId, webPage);
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

    DeclarativeWebPage *newActiveWebPage = m_activePages.activate(tabId);
    updateStates(oldActiveWebPage, newActiveWebPage);

#if DEBUG_LOGS
    dumpPages();
#endif

    return WebPageActivationData(newActiveWebPage, true);
}

void WebPages::release(int tabId, bool virtualize)
{
    m_activePages.release(tabId, virtualize);
}

void WebPages::clear()
{
    m_activePages.clear();
}

int WebPages::parentTabId(int tabId) const
{
    return m_activePages.parentTabId(tabId);
}

void WebPages::updateStates(DeclarativeWebPage *oldActivePage, DeclarativeWebPage *newActivePage)
{
    if (oldActivePage) {
        oldActivePage->setVisible(false);

        // Allow suspending only the current active page if it is not the creator (parent).
        if (newActivePage->parentId() != (int)oldActivePage->uniqueID()) {
             if (oldActivePage->loading()) {
                 oldActivePage->stop();
             }
             oldActivePage->suspendView();
        }
    }

    if (newActivePage) {
        newActivePage->resumeView();
        newActivePage->setVisible(true);
    }
}

void WebPages::dumpPages() const
{
    m_activePages.dumpPages();
}
