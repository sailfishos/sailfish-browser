/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "link.h"
#include <QDebug>

Link::Link(int linkId, const QString &urlString, const QString &thumbPath, const QString &title, const QDate &date) :
    m_linkId(linkId), m_url(urlString), m_thumbPath(thumbPath), m_title(title), m_date(date)
{
}

Link::Link() :
    m_linkId(0), m_url(""), m_thumbPath(""), m_title(""), m_date(QDate())
{
}

int Link::linkId() const
{
    return m_linkId;
}

void Link::setLinkId(int linkId)
{
    m_linkId = linkId;
}

QString Link::url() const
{
    return m_url;
}

void Link::setUrl(const QString &url)
{
    m_url = url;
}

QString Link::thumbPath() const
{
    return m_thumbPath;
}

void Link::setThumbPath(const QString &thumbPath)
{
    m_thumbPath = thumbPath;
}

QString Link::title() const
{
    return m_title;
}

void Link::setTitle(const QString &title)
{
    m_title = title;
}

bool Link::isValid() const
{
    return m_linkId > 0 && m_url.length() > 0;
}

bool Link::operator==(const Link &other) const
{
    return (m_linkId == other.linkId()
            && m_url == other.url()
            && m_thumbPath == other.thumbPath()
            && m_title == other.title()
            && m_date == other.date());
}

bool Link::operator!=(const Link &other) const
{
    return !(*this == other);
}

QDate Link::date() const
{
    return m_date;
}

void Link::setDate(const QDate &date)
{
    m_date = date;
}

QDebug operator<<(QDebug dbg, const Link *link) {
    if (!link) {
        return dbg << "Link (this = 0x0)";
    }

    dbg.nospace() << "Link(linkId = " << link->linkId() << ", isValid = " << link->isValid()
                  << ", url = " << link->url() << ", title = " << link->title()
                  << ", thumbnailPath = " << link->thumbPath() << ", date = " << link->date() << ")";
    return dbg.space();
}
