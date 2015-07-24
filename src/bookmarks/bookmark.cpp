/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bookmark.h"

Bookmark::Bookmark(QString title, QString url, QString favicon, bool hasTouchIcon, QObject* parent)
    : QObject(parent)
    , m_title(title)
    , m_url(url)
    , m_favicon(favicon)
    , m_hasTouchIcon(hasTouchIcon)
{
    if (m_favicon.isEmpty()) {
        m_favicon = DEFAULT_DESKTOP_BOOKMARK_ICON;
        m_hasTouchIcon = true;
    }
}

QString Bookmark::title() const {
    return m_title;
}

void Bookmark::setTitle(QString title) {
    if(title != m_title) {
        m_title = title;
        emit titleChanged();
    }
}

QString Bookmark::url() const {
    return m_url;
}

void Bookmark::setUrl(QString url) {
    if(url != m_url) {
        m_url = url;
        emit urlChanged();
    }
}

QString Bookmark::favicon() const {
    return m_favicon;
}

void Bookmark::setFavicon(QString favicon) {
    if(favicon != m_favicon) {
        m_favicon = favicon;
        emit faviconChanged();
    }
}

bool Bookmark::hasTouchIcon() const
{
    return m_hasTouchIcon;
}

void Bookmark::setHasTouchIcon(bool hasTouchIcon)
{
    m_hasTouchIcon = hasTouchIcon;
}
