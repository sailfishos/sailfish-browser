/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BROWSERPATHS_H
#define BROWSERPATHS_H

class QString;

struct BrowserPaths
{
    static QString downloadLocation();
    static QString picturesLocation();
    static QString dataLocation();
    static QString applicationsLocation();
    static QString cacheLocation();
};

#endif // BROWSERPATHS_H
