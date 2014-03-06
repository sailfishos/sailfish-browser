/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <qmozcontext.h>
#include <quickmozview.h>
#include "declarativewebviewcreator.h"

DeclarativeWebViewCreator::DeclarativeWebViewCreator(QObject *parent)
    : QMozViewCreator(parent)
    , m_activeWebView(0)
{
    QMozContext::GetInstance()->setViewCreator(this);
}


DeclarativeWebViewCreator::~DeclarativeWebViewCreator()
{
    QMozContext::GetInstance()->setViewCreator(0);
}


QuickMozView *DeclarativeWebViewCreator::activeWebView() const
{
    return m_activeWebView;
}


void DeclarativeWebViewCreator::setActiveWebView(QuickMozView *activeWebView)
{
    if (m_activeWebView != activeWebView) {
        m_activeWebView = activeWebView;
        emit activeWebViewChanged();
    }
}


quint32 DeclarativeWebViewCreator::createView(const QString &url, const quint32 &parentId)
{
    QPointer<QuickMozView> oldView = m_activeWebView;
    emit newWindowRequested(url, parentId);
    if (m_activeWebView && oldView != m_activeWebView) {
        return m_activeWebView->uniqueID();
    }
    return 0;
}
