/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include "declarativewebthumbnail.h"

DeclarativeWebThumbnail::DeclarativeWebThumbnail(QString path, QObject *parent) :
    QObject(parent),
    m_path(path),
    m_ready(false)
{
}

QString DeclarativeWebThumbnail::path()
{
    if (m_ready) {
        return m_path;
    } else {
        return "image://theme/icon-m-region";
    }
}

QString DeclarativeWebThumbnail::source()
{
    return m_path;
}

void DeclarativeWebThumbnail::setReady(bool ready)
{
    if (m_ready != ready) {
        m_ready = ready;
        emit pathChanged();
    }
}
