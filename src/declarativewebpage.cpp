/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativewebpage.h"

static const QString gFullScreenMessage("embed:fullscreenchanged");

DeclarativeWebPage::DeclarativeWebPage(QuickMozView *parent)
    : QuickMozView(parent)
    , m_container(0)
    , m_tabId(0)
    , m_viewReady(false)
    , m_loaded(false)
    , m_userHasDraggedWhileLoading(false)
    , m_fullscreen(false)
    , m_deferredReload(false)
{
    connect(this, SIGNAL(viewInitialized()), this, SLOT(onViewInitialized()));
    connect(this, SIGNAL(recvAsyncMessage(const QString, const QVariant)),
            this, SLOT(onRecvAsyncMessage(const QString&, const QVariant&)));
}

DeclarativeWebPage::~DeclarativeWebPage()
{

}

DeclarativeWebContainer *DeclarativeWebPage::container() const
{
    return m_container;
}

void DeclarativeWebPage::setContainer(DeclarativeWebContainer *container)
{
    if (m_container != container) {
        m_container = container;
        emit containerChanged();
    }
}

int DeclarativeWebPage::tabId() const
{
    return m_tabId;
}

void DeclarativeWebPage::setTabId(int tabId)
{
    m_tabId = tabId;
}

QVariant DeclarativeWebPage::resurrectedContentRect() const
{
    return m_resurrectedContentRect;
}

void DeclarativeWebPage::setResurrectedContentRect(QVariant resurrectedContentRect)
{
    if (m_resurrectedContentRect != resurrectedContentRect) {
        m_resurrectedContentRect = resurrectedContentRect;
        emit resurrectedContentRectChanged();
    }
}

void DeclarativeWebPage::componentComplete()
{
    QuickMozView::componentComplete();
}

void DeclarativeWebPage::onViewInitialized()
{
    addMessageListener(gFullScreenMessage);
}

void DeclarativeWebPage::onRecvAsyncMessage(const QString& message, const QVariant& data)
{
    if (message == gFullScreenMessage) {
        setFullscreen(data.toMap().value(QString("fullscreen")).toBool());
    }
}

bool DeclarativeWebPage::fullscreen() const
{
    return m_fullscreen;
}

void DeclarativeWebPage::setFullscreen(const bool fullscreen)
{
    if (m_fullscreen != fullscreen) {
        m_fullscreen = fullscreen;
        m_container->resetHeight();
        emit fullscreenChanged();
    }
}
