/****************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <webengine.h>
#include "declarativewebpage.h"
#include "declarativewebpagecreator.h"
#include "declarativetabmodel.h"

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

DeclarativeTabModel *DeclarativeWebPageCreator::model() const
{
    return m_model;
}

void DeclarativeWebPageCreator::setModel(DeclarativeTabModel *model)
{
    if (m_model != model) {
        m_model = model;
        emit modelChanged();
    }
}

quint32 DeclarativeWebPageCreator::createView(const quint32 &parentId, const uintptr_t &parentBrowsingContext)
{
    QPointer<DeclarativeWebPage> oldPage = m_activeWebPage;
    m_model->newTab(QString(), parentId, parentBrowsingContext);

    if (m_activeWebPage && oldPage != m_activeWebPage) {
        return m_activeWebPage->uniqueId();
    }
    return 0;
}
