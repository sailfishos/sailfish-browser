/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

import QtQuick 2.0

Item {
    // providing dummy translations that can be used in settings entry files
    function qsTrIdString() {
        //% "Browser"
        QT_TRID_NOOP("settings_applications-browser")
    }
}
