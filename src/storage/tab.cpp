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

Tab::Tab(int tabId, QString url, QString title, QString thumbPath) :
    m_tabId(tabId), m_url(url), m_title(title), m_thumbPath(thumbPath)
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
    return m_url;
}

void Tab::setUrl(const QString &url)
{
    m_url = url;
}

QString Tab::thumbnailPath() const
{
    return m_thumbPath;
}

void Tab::setThumbnailPath(const QString &thumbnailPath)
{
    m_thumbPath = thumbnailPath;
}

QString Tab::title() const
{
    return m_title;
}

void Tab::setTitle(const QString &title)
{
    m_title = title;
}

bool Tab::isValid() const
{
    return m_tabId > 0;
}

bool Tab::operator==(const Tab &other) const
{
    return (m_tabId == other.tabId() &&
            m_url == other.url() &&
            m_title == other.title() &&
            m_thumbPath == other.thumbnailPath());
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
