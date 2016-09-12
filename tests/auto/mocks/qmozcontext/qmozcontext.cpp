/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qmozcontext.h"

static QMozContext* gSingleton = 0;

QMozContext* QMozContext::instance()
{
    if (!gSingleton) {
        gSingleton = new QMozContext();
    }
    return gSingleton;
}
