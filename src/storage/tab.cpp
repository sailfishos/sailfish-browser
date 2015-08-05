/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "tab.h"

Tab::Tab(int tabId, Link link) :
    m_tabId(tabId), m_link(link)
{
}

Tab::Tab() :
    m_tabId(0)
{
}

int Tab::tabId() const
{
    return m_tabId;
}

void Tab::setTabId(int tabId)
{
    m_tabId = tabId;
}

QString Tab::url() const
{
    return m_link.url();
}

void Tab::setUrl(const QString &url)
{
    m_link.setUrl(url);
}

QString Tab::thumbnailPath() const
{
    return m_link.thumbPath();
}

void Tab::setThumbnailPath(const QString &thumbnailPath)
{
    m_link.setThumbPath(thumbnailPath);
}

QString Tab::title() const
{
    return m_link.title();
}

void Tab::setTitle(const QString &title)
{
    m_link.setTitle(title);
}

bool Tab::isValid() const
{
    return m_tabId > 0;
}

bool Tab::operator==(const Tab &other) const
{
    return (m_tabId == other.tabId() &&
            m_link == other.m_link);
}

bool Tab::operator!=(const Tab &other) const
{
    return !(*this == other);
}

QDebug operator<<(QDebug dbg, const Tab *tab) {
    if (!tab) {
        return dbg << "Tab (this = 0x0)";
    }

    dbg.nospace() << "Tab(tabId = " << tab->tabId() << ", isValid = " << tab->isValid()
                  << ", url = " << tab->url() << ", title = " << tab->title() << ", thumbnailPath = " << tab->thumbnailPath() << ")";
    return dbg.space();
}
