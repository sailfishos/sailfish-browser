/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include "declarativewebutils.h"

DeclarativeWebUtils::DeclarativeWebUtils(QObject *parent) :
    QObject(parent)
{
}

QUrl DeclarativeWebUtils::getFaviconForUrl(QUrl url) {
    QUrl faviconUrl(url);
    faviconUrl.setPath("/favicon.ico");
    return faviconUrl;
}
