/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>

#include <qqmlinfo.h>

#include "webpagefactory.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "tab.h"

#define DEBUG_LOGS 0

DeclarativeWebPage* WebPageFactory::createWebPage(DeclarativeWebContainer *webContainer,
                                                  const Tab &initialTab,
                                                  int parentId)
{
    if (!m_qmlComponent) {
        qWarning() << "WebPageContainer not initialized!";
        return nullptr;
    }

    QQmlContext *creationContext = m_qmlComponent->creationContext();
    QQmlContext *context = new QQmlContext(creationContext ? creationContext : QQmlEngine::contextForObject(webContainer));
    QObject *object = m_qmlComponent->beginCreate(context);
    if (object) {
        context->setParent(object);
        object->setParent(webContainer);
        DeclarativeWebPage* webPage = qobject_cast<DeclarativeWebPage *>(object);
        if (webPage) {
            webPage->setParentID(parentId);
            webPage->setPrivateMode(webContainer->privateMode());
            webPage->setInitialTab(initialTab);
            webPage->setContainer(webContainer);
            webPage->initialize();
            m_qmlComponent->completeCreate();
#if DEBUG_LOGS
            qDebug() << "New view id:" << webPage->uniqueID() << "parentId:" << webPage->parentId() << "tab id:" << webPage->tabId();
#endif
            QQmlEngine::setObjectOwnership(webPage, QQmlEngine::CppOwnership);
            return webPage;
        } else {
            qmlInfo(webContainer) << "webPage component must be a WebPage component";
            m_qmlComponent->completeCreate();
            delete object;
        }
    } else {
        qmlInfo(webContainer) << "Creation of the web page failed. Error: " << m_qmlComponent->errorString();
        delete context;
    }

    return nullptr;
}

void WebPageFactory::updateQmlComponent(QQmlComponent *newComponent)
{
    m_qmlComponent = newComponent;
}
