/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0

QtObject {
    // providing dummy translations that are used in the captive portal
    function qsTrIdString() {
        //: Shown when sign in to captive portal
        //% "Sign in"
        QT_TRID_NOOP("sailfish_browser-la-sign_in")
    }
}
