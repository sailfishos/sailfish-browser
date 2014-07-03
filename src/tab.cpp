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

Tab::Tab(int tabId, Link currentLink, int nextLinkId, int previousLinkId)
    : m_tabId(tabId)
    , m_currentLink(currentLink)
    , m_nextLinkId(nextLinkId)
    , m_previousLinkId(previousLinkId)
    , m_bookmarked(false)
{
}

Tab::Tab() :
    m_tabId(0), m_nextLinkId(0), m_previousLinkId(0)
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

int Tab::currentLink() const
{
    return m_currentLink.linkId();
}

void Tab::setCurrentLink(int currentLinkId)
{
    m_currentLink.setLinkId(currentLinkId);
}

int Tab::nextLink() const
{
    return m_nextLinkId;
}

void Tab::setNextLink(int nextLinkId)
{
    m_nextLinkId = nextLinkId;
}

QString Tab::url() const
{
    return m_currentLink.url();
}

void Tab::setUrl(const QString &url)
{
    m_currentLink.setUrl(url);
}

QString Tab::thumbnailPath() const
{
    return m_currentLink.thumbPath();
}

void Tab::setThumbnailPath(const QString &thumbnailPath)
{
    m_currentLink.setThumbPath(thumbnailPath);
}

QString Tab::title() const
{
    return m_currentLink.title();
}

void Tab::setTitle(const QString &title)
{
    m_currentLink.setTitle(title);
}

QString Tab::favoriteIcon() const
{
    return m_favoriteIcon;
}

void Tab::setFavoriteIcon(const QString &icon)
{
    m_favoriteIcon = icon;
}

bool Tab::bookmarked() const
{
    return m_bookmarked;
}

void Tab::setBookmarked(bool bookmarked)
{
    m_bookmarked = bookmarked;
}

bool Tab::isValid() const
{
    return m_tabId > 0;
}

int Tab::previousLink() const
{
    return m_previousLinkId;
}

void Tab::setPreviousLink(int previousLinkId)
{
    m_previousLinkId = previousLinkId;
}

bool Tab::operator==(const Tab &other) const
{
    return (m_tabId == other.tabId() &&
            m_previousLinkId == other.m_previousLinkId &&
            m_nextLinkId == other.m_nextLinkId &&
            m_currentLink == other.m_currentLink);
}

bool Tab::operator!=(const Tab &other) const
{
    return !(*this == other);
}

QDebug operator<<(QDebug dbg, const Tab *tab) {
    if (!tab) {
        return dbg << "Tab (this = 0x0)";
    }

    dbg.nospace() << "Tab(tabId = " << tab->tabId() << ", isValid = " << tab->isValid() << ", linkId = " << tab->currentLink()
                  << ", previousLink = " << tab->previousLink() << ", nextLink = " << tab->nextLink()
                  << ", url = " << tab->url() << ", title = " << tab->title() << ", thumbnailPath = " << tab->thumbnailPath() << ")";
    return dbg.space();
}
