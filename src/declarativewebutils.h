/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#ifndef DECLARATIVEWEBUTILS_H
#define DECLARATIVEWEBUTILS_H

#include <QObject>
#include <QUrl>

class DeclarativeWebUtils : public QObject
{
    Q_OBJECT
public:
    explicit DeclarativeWebUtils(QObject *parent = 0);

    Q_INVOKABLE QUrl getFaviconForUrl(QUrl url);
};
#endif // DECLARATIVEWEBUTILS_H
