/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QCoreApplication>
#include <QString>
#include "browserapp.h"

bool BrowserApp::captivePortal()
{
    static bool captivePortalMode = false;
    static bool argsChecked = false;

    if (!argsChecked) {
        if (QCoreApplication::arguments().contains(QStringLiteral("-captiveportal")))
            captivePortalMode = true;
        argsChecked = true;
    }

    return captivePortalMode;
}

QString BrowserApp::profileName()
{
    static QString profileName = QStringLiteral("mozembed");
    static bool argsChecked = false;

    if (!argsChecked) {
        const QStringList &arguments = QCoreApplication::arguments();

        if (arguments.contains(QStringLiteral("-profile"))) {
            int index = arguments.indexOf(QStringLiteral("-profile"));
            if (index + 1 < arguments.size()) {
                profileName = arguments.at(index + 1);
            }
        } else if (BrowserApp::captivePortal()) {
            profileName = QStringLiteral("captiveportal");
        }
    }
    return profileName;
}
