/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1

QtObject {
    property Item webView
    property bool videoActive
    property bool audioActive
    property bool background

    signal webViewSuspended

    function calculateStatus() {}

    function resumeView() {}

    function suspendView() {}
}
