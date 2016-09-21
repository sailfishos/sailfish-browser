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

#include <qmozcontext.h>

static DeclarativeWebUtils *gSingleton = 0;

DeclarativeWebUtils::DeclarativeWebUtils(QObject *parent)
    : QObject(parent)
    , m_homePage("file:///opt/tests/sailfish-browser/manual/testpage.html")
    , m_firstUseDone(true)
{
    connect(QMozContext::instance(), SIGNAL(onInitialized()),
            this, SLOT(updateWebEngineSettings()));
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

void DeclarativeWebUtils::updateWebEngineSettings()
{
    QMozContext* mozContext = QMozContext::instance();

    // Add only mandatory preferences for unit tests.

    // Don't force 16bit color depth
    mozContext->setPref(QString("gfx.qt.rgb16.force"), QVariant(false));

    // Use external Qt window for rendering content
    mozContext->setPref(QString("gfx.compositor.external-window"), QVariant(true));
    mozContext->setPref(QString("gfx.compositor.clear-context"), QVariant(false));
    mozContext->setPref(QString("embedlite.compositor.external_gl_context"), QVariant(true));
    mozContext->setPref(QString("embedlite.compositor.request_external_gl_context_early"), QVariant(true));

    // Progressive painting.
    mozContext->setPref(QString("layers.progressive-paint"), QVariant(true));
    mozContext->setPref(QString("layers.low-precision-buffer"), QVariant(true));

    // Set max active layers
    mozContext->setPref(QString("layers.max-active"), QVariant(20));
}

DeclarativeWebUtils *DeclarativeWebUtils::instance()
{
    if (!gSingleton) {
        gSingleton = new DeclarativeWebUtils();
    }
    return gSingleton;
}

