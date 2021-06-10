/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import com.jolla.settings.system 1.0
import org.nemomobile.devicelock 1.0

Authenticator {
    id: deviceLockQuery
    property bool requireAuthentication: true
    property bool available: availableMethods !== Authenticator.NoAuthentication
    property string message
    property var _resolve

    function perform(resolve) {
        if (requireAuthentication && available) {
            _resolve = resolve

            deviceLockQuery.requestPermission(message, {})
        } else {
            resolve()
        }
    }

    onPermissionGranted: {
        if (_resolve) {
            requireAuthentication = false
            _resolve()
        }
    }
}
