/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "browsersettings.h"

BrowserSettings::BrowserSettings(QObject *parent)
    : QObject(parent)
{
}

BrowserSettings::~BrowserSettings()
{
}

const QString BrowserSettings::testProp() const
{
    return QString("test property");
}
