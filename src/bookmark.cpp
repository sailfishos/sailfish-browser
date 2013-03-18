/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include "bookmark.h"

Bookmark::Bookmark(QString title, QString url, QString favicon, QObject* parent) :
    QObject(parent), m_title(title), m_url(url), m_favicon(favicon)
{
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
