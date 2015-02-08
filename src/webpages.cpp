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
#include "qmozcontext.h"

#include <QDateTime>
#include <QDBusConnection>
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

static const qint64 gMemoryPressureTimeout = 600 * 1000; // 600 sec
// In normal cases gLowMemoryEnabled is true. Can be disabled e.g. for test runs.
static const bool gLowMemoryEnabled = qgetenv("LOW_MEMORY_DISABLED").isEmpty();

WebPages::WebPages(QObject *parent)
    : QObject(parent)
    , m_backgroundTimestamp(0)
{
    if (gLowMemoryEnabled) {
        QDBusConnection::systemBus().connect("com.nokia.mce", "/com/nokia/mce/signal",
                                             "com.nokia.mce.signal", "sig_memory_level_ind",
                                             this, SLOT(handleMemNotify(QString)));
    }
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

    connect(webContainer, SIGNAL(foregroundChanged()), this, SLOT(updateBackgroundTimestamp()));
}

void WebPages::updateBackgroundTimestamp()
{
    if (!m_webContainer->foreground()) {
        m_backgroundTimestamp = QDateTime::currentMSecsSinceEpoch();
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

bool WebPages::alive(int tabId) const
{
    return m_activePages.alive(tabId);
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
                webPage->setPrivateMode(m_webContainer->privateMode());
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
        oldActivePage->setOpacity(1.0);

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

void WebPages::handleMemNotify(const QString &memoryLevel)
{
    if (!m_webContainer || !m_webContainer->completed()) {
        return;
    }

    if (memoryLevel == QString("warning") || memoryLevel == QString("critical")) {
        m_activePages.virtualizeInactive();

        if (!m_webContainer->foreground() &&
                (QDateTime::currentMSecsSinceEpoch() - m_backgroundTimestamp) > gMemoryPressureTimeout) {
            m_backgroundTimestamp = QDateTime::currentMSecsSinceEpoch();
            QMozContext::GetInstance()->sendObserve(QString("memory-pressure"), QString("heap-minimize"));
        }
    }
}
