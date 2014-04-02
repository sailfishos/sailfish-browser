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

Tab::Tab(int tabId, Link currentLink, int nextLinkId, int previousLinkId) :
    m_tabId(tabId), m_currentLink(currentLink), m_nextLinkId(nextLinkId), m_previousLinkId(previousLinkId)
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


int Tab::nextLink() const
{
    return m_nextLinkId;
}

void Tab::setNextLink(int nextLinkId)
{
    m_nextLinkId = nextLinkId;
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

Link Tab::currentLink() const
{
    return m_currentLink;
}

void Tab::setCurrentLink(const Link &currentLink)
{
    m_currentLink = currentLink;
}

bool Tab::operator==(const Tab &other) const
{
    return (m_tabId == other.tabId() &&
            m_previousLinkId == other.previousLink() &&
            m_nextLinkId == other.nextLink() &&
            m_currentLink == other.currentLink());
}

bool Tab::operator!=(const Tab &other) const
{
    return !(*this == other);
}

QDebug operator<<(QDebug dbg, const Tab *tab) {
    if (!tab) {
        return dbg << "Tab (this = 0x0)";
    }

    Link currentLink = tab->currentLink();
    dbg.nospace() << "Tab(tabId = " << tab->tabId() << ", isValid = " << tab->isValid() << ", linkId = " << currentLink.linkId()
                  << ", previousLink = " << tab->previousLink() << ", nextLink = " << tab->nextLink()
                  << ", url = " << currentLink.url() << ", title = " << currentLink.title() << ", thumbnailPath = " << currentLink.thumbPath() << ")";
    return dbg.space();
}
