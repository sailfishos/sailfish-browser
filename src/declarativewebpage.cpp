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

DeclarativeWebPage::DeclarativeWebPage(QuickMozView *parent)
    : QuickMozView(parent)
    , m_container(0)
    , m_tabId(0)
    , m_viewReady(false)
    , m_loaded(false)
    , m_userHasDraggedWhileLoading(false)
    , m_deferredReload(false)
{

}

DeclarativeWebPage::~DeclarativeWebPage()
{

}

void DeclarativeWebPage::componentComplete()
{
    QuickMozView::componentComplete();
}
