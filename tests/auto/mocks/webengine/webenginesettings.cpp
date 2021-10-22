/****************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelaine@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "webenginesettings.h"

static SailfishOS::WebEngineSettings *gSingleton = 0;

SailfishOS::WebEngineSettings *SailfishOS::WebEngineSettings::instance()
{
    if (!gSingleton) {
        gSingleton = new SailfishOS::WebEngineSettings();
    }
    return gSingleton;
}

void SailfishOS::WebEngineSettings::initialize()
{
}
