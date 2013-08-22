/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#include "link.h"

Link::Link(int linkId, QString urlString, QString thumbPath, QString title) :
    m_linkId(linkId), m_url(urlString), m_thumbPath(thumbPath), m_title(title)
{
}

Link::Link() :
    m_linkId(0), m_url(""), m_thumbPath(""), m_title("")
{
}

Link::Link(const Link& l) :
    m_linkId(l.m_linkId), m_url(l.m_url), m_thumbPath(l.m_thumbPath), m_title(l.m_title)
{
}

int Link::linkId() const
{
    return m_linkId;
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
    return (m_linkId == other.linkId() && m_url == other.url() && m_thumbPath == other.thumbPath() && m_title == other.title());
}

bool Link::operator!=(const Link &other) const
{
    return !(*this == other);
}
