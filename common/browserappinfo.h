/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BROWSERAPPINFO_H
#define BROWSERAPPINFO_H

#include <QString>

namespace BrowserAppInfo
{
    bool captivePortal();
    bool sparse();
    QString profileName();
};

#endif // BROWSERAPPINFO_H
