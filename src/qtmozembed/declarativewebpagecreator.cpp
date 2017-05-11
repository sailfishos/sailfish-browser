/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <webengine.h>
#include "declarativewebpage.h"
#include "declarativewebpagecreator.h"

DeclarativeWebPageCreator::DeclarativeWebPageCreator(QObject *parent)
    : QMozViewCreator(parent)
    , m_activeWebPage(0)
{
    SailfishOS::WebEngine::instance()->setViewCreator(this);
}

DeclarativeWebPageCreator::~DeclarativeWebPageCreator()
{
    SailfishOS::WebEngine::instance()->setViewCreator(0);
}

DeclarativeWebPage *DeclarativeWebPageCreator::activeWebPage() const
{
    return m_activeWebPage;
}

void DeclarativeWebPageCreator::setActiveWebPage(DeclarativeWebPage *activeWebPage)
{
    if (m_activeWebPage != activeWebPage) {
        m_activeWebPage = activeWebPage;
        emit activeWebPageChanged();
    }
}

quint32 DeclarativeWebPageCreator::createView(const QString &url, const quint32 &parentId)
{
    QPointer<DeclarativeWebPage> oldPage = m_activeWebPage;
    emit newWindowRequested(url, parentId);
    if (m_activeWebPage && oldPage != m_activeWebPage) {
        return m_activeWebPage->uniqueID();
    }
    return 0;
}
