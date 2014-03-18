/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBUTILS_MOCK
#define WEBUTILS_MOCK

#include <QObject>
#include <QString>
#include <QColor>
#include <qqml.h>

class WebUtilsMock : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString homePage MEMBER homePage NOTIFY homePageChanged FINAL)

public:
    explicit WebUtilsMock(QObject *parent = 0);

    Q_INVOKABLE int getLightness(QColor color) const;
    Q_INVOKABLE QString displayableUrl(QString fullUrl) const;

    static QObject *singletonApiFactory(QQmlEngine *, QJSEngine *);
    static WebUtilsMock *instance();

    QString homePage;

signals:
    void homePageChanged();
};

QML_DECLARE_TYPE(WebUtilsMock)

#endif
