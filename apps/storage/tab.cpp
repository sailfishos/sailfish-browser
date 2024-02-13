/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "tab.h"

Tab::Tab(int tabId, const QString &url, const QString &title, const QString &thumbPath, const bool hidden)
    : m_tabId(tabId)
    , m_requestedUrl(url)
    , m_url(url)
    , m_title(title)
    , m_thumbPath(thumbPath)
    , m_desktopMode(false)
    , m_hidden(hidden)
    , m_browsingContext(0)
    , m_parentId(0)
{
}

Tab::Tab()
    : m_tabId(0)
    , m_desktopMode(false)
    , m_hidden(false)
    , m_browsingContext(0)
    , m_parentId(0)
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

void Tab::setRequestedUrl(const QString &url)
{
    m_requestedUrl = url;
}

QString Tab::requestedUrl() const
{
    return m_requestedUrl;
}

QString Tab::url() const
{
    return !m_requestedUrl.isEmpty() && !hasResolvedUrl() ? m_requestedUrl : m_url;
}

void Tab::setUrl(const QString &url)
{
    m_url = url;
}

bool Tab::hasResolvedUrl() const
{
    return !m_url.isEmpty();
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

bool Tab::desktopMode() const
{
    return m_desktopMode;
}

void Tab::setDesktopMode(bool desktopMode)
{
    m_desktopMode = desktopMode;
}

void Tab::setBrowsingContext(uintptr_t browsingContext)
{
    Q_ASSERT_X(m_browsingContext == 0, Q_FUNC_INFO, "Browsing context can be set only once.");
    m_browsingContext = browsingContext;
}

uintptr_t Tab::browsingContext() const
{
    return m_browsingContext;
}

void Tab::setParentId(uint32_t parentId)
{
    Q_ASSERT_X(m_parentId == 0, Q_FUNC_INFO, "Parent id can be set only once.");
    m_parentId = parentId;
}

uint32_t Tab::parentId() const
{
    return m_parentId;
}

bool Tab::hidden() const
{
    return m_hidden;
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

    dbg.nospace() << "Tab(tabId = " << tab->tabId() << ", parentId = " << tab->parentId()
                  << ", isValid = " << tab->isValid()
                  << ", url = " << tab->url() << ", requested url = " << tab->requestedUrl()
                  << ", url resolved: " << tab->hasResolvedUrl() << ", title = " << tab->title()
                  << ", thumbnailPath = " << tab->thumbnailPath()
                  << ", desktopMode = " << tab->desktopMode() << ")";
    return dbg.space();
}
