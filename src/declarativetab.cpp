/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativetab.h"

DeclarativeTab::DeclarativeTab(QObject *parent)
    : QObject(parent)
{}

DeclarativeTab::~DeclarativeTab() {}

QString DeclarativeTab::thumbnailPath() const
{
    return m_link.thumbPath();
}

void DeclarativeTab::setThumbnailPath(QString thumbPath)
{
    if (m_tab.isValid() && m_link.thumbPath() != thumbPath) {
        m_link.setThumbPath(thumbPath);
        emit thumbPathChanged(thumbPath, m_tab.tabId());
    }
}

QString DeclarativeTab::url() const
{
    return m_link.url();
}

void DeclarativeTab::setUrl(QString url)
{
    if (m_tab.isValid() && m_link.url() != url) {
        m_link.setUrl(url);
        emit urlChanged();
    }
}

QString DeclarativeTab::title() const
{
    return m_link.title();
}

void DeclarativeTab::setTitle(QString title)
{
    if (m_tab.isValid() && m_link.title() != title) {
        m_link.setTitle(title);
        emit titleChanged();
    }
}

int DeclarativeTab::tabId() const
{
    return m_tab.tabId();
}

bool DeclarativeTab::valid() const
{
    return m_tab.isValid();
}

void DeclarativeTab::setInvalid()
{
    if (m_tab.isValid()) {
        m_tab.setTabId(0);
        emit validChanged();
    }
}

void DeclarativeTab::invalidateTabData()
{
    int tabId = m_tab.tabId();
    setInvalid();
    if (!m_link.thumbPath().isEmpty()) {
        m_link.setThumbPath("");
        emit thumbPathChanged("", tabId);
    }

    if (!m_link.title().isEmpty()) {
        m_link.setTitle("");
        emit titleChanged();
    }

    if (!m_link.url().isEmpty()) {
        m_link.setUrl("");
        emit urlChanged();
    }
}

Tab DeclarativeTab::tabData() const
{
    return m_tab;
}

int DeclarativeTab::linkId() const
{
    return m_link.linkId();
}

void DeclarativeTab::updateTabData(const Tab &tab)
{
    Link newLink = tab.currentLink();

    bool thumbChanged = m_link.thumbPath() != newLink.thumbPath();
    bool titleStringChanged = m_link.title() != newLink.title();
    bool urlStringChanged = m_link.url() != newLink.url();

#ifdef DEBUG_LOGS
    qDebug() << "old values:" << &m_tab;
    qDebug() << "new values:" << &tab;
#endif

    bool validValueChanged = m_tab.isValid() != tab.isValid();
    if (validValueChanged || urlStringChanged || thumbChanged || titleStringChanged || m_link.linkId() != newLink.linkId()) {
        m_tab = tab;
        m_link = newLink;
    }

    if (validValueChanged) {
        emit validChanged();
    }
    if (urlStringChanged) {
        emit urlChanged();
    }
    if (thumbChanged) {
        emit thumbPathChanged(newLink.thumbPath(), tab.tabId());
    }
    if (titleStringChanged) {
        emit titleChanged();
    }
}

void DeclarativeTab::activatePreviousLink()
{
    int previousLink = m_tab.previousLink();
    if (previousLink > 0) {
        m_link = Link(previousLink, url(), thumbnailPath(), title());
    }
}

void DeclarativeTab::activateNextLink()
{
    int nextLink = m_tab.nextLink();
    if (nextLink > 0) {
        m_link = Link(nextLink, url(), thumbnailPath(), title());
    }
}

QDebug operator<<(QDebug dbg, const DeclarativeTab *tab) {
    if (!tab) {
        return dbg << "DeclarativeTab (this = 0x0)";
    }
    dbg.nospace() << "DeclarativeTab(tabId = " << tab->tabId() << ", valid = " << tab->valid() << ", linkId = " << tab->linkId()
                  << ", url = " << tab->url() << ", title = " << tab->title() << ", thumbnailPath = " << tab->thumbnailPath() << ")";
    return dbg.space();
}

