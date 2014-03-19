/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "webUtilsMock.h"

static WebUtilsMock *s_instance;

WebUtilsMock::WebUtilsMock(QObject *parent)
    : QObject(parent)
    , homePage("file:///opt/tests/sailfish-browser/manual/testpage.html")
{
}

int WebUtilsMock::getLightness(QColor color) const
{
    return color.lightness();
}

QString WebUtilsMock::displayableUrl(QString fullUrl) const
{
    return fullUrl;
}

QObject *WebUtilsMock::singletonApiFactory(QQmlEngine *, QJSEngine *)
{
    // Instance will be owned by QML engine.
    s_instance = new WebUtilsMock();
    return s_instance;
}

WebUtilsMock *WebUtilsMock::instance()
{
    if (!s_instance) {
        s_instance = new WebUtilsMock();
    }
    return s_instance;
}
