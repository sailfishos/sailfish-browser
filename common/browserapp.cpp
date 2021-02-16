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
#include <QStandardPaths>
#include "browserapp.h"

bool BrowserApp::captivePortal()
{
    static bool captivePortalMode = false;
    static bool argsChecked = false;

    if (!argsChecked) {
        if (QCoreApplication::arguments().contains(QLatin1String("-captiveportal")))
            captivePortalMode = true;
        argsChecked = true;
    }

    return captivePortalMode;
}

QString BrowserApp::profileName()
{
    const QStringList &arguments = QCoreApplication::arguments();
    int index = arguments.indexOf(QLatin1String("-profile"));
    if (index >= 0 && index + 1 < arguments.size()) {
        return arguments.at(index + 1);
    }
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}
