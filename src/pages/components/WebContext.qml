/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0

Connections {
    property Item webView
    property bool firstFrameRendered

    target: MozContext
    onRecvObserve: {
        if (message === "embedlite-before-first-paint") {
            firstFrameRendered = true
        }
    }
}
