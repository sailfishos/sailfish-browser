/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Open Mobile Platform LLC.
 */

import QtQuick 2.1

QtObject {
    property QtObject webPage
    property bool videoActive
    property bool audioActive
    property bool background
    property bool displayOff

    function calculateStatus() {}

    function resumeView() {}

    function suspendView() {}
}
