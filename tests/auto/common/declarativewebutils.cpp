/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativewebutils.h"

static DeclarativeWebUtils *gSingleton = 0;

DeclarativeWebUtils::DeclarativeWebUtils(QObject *parent)
    : QObject(parent)
    , m_homePage("file:///opt/tests/sailfish-browser/manual/testpage.html")
    , m_firstUseDone(true)
{
}

int DeclarativeWebUtils::getLightness(QColor color) const
{
    return color.lightness();
}

QString DeclarativeWebUtils::displayableUrl(QString fullUrl) const
{
    return fullUrl;
}

QString DeclarativeWebUtils::homePage() const
{
    return m_homePage;
}

bool DeclarativeWebUtils::firstUseDone() const
{
    return m_firstUseDone;
}

void DeclarativeWebUtils::setFirstUseDone(bool firstUseDone)
{
    m_firstUseDone = firstUseDone;
}

qreal DeclarativeWebUtils::silicaPixelRatio() const
{
    return 1.0;
}

DeclarativeWebUtils *DeclarativeWebUtils::instance()
{
    if (!gSingleton) {
        gSingleton = new DeclarativeWebUtils();
    }
    return gSingleton;
}
