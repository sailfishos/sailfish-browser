/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/
import QtQuick 1.1
import Sailfish.Silica 1.0

Item {
    id: progressBar

    // From 0 to 1.0
    property real progress: 0.0
    property string title: ""
    property string cancelText: qsTrId("components-la-tap-to-cancel")

    signal stopped
    visible: opacity > 0.0

    Rectangle {
        id: progressRect
        anchors {
            top: parent.top
            left: parent.left
        }
        height: parent.height
        width: progressBar.progress * parent.width
        color: theme.highlightBackgroundColor
        opacity: 0.3
    }

    Rectangle {
        anchors {
            top: parent.top
            left: progressRect.right
            right: parent.right
        }
        height: parent.height
        opacity: 0.15
        color: "black"
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: theme.paddingLarge
        anchors.verticalCenter: parent.verticalCenter

        Label {
            id: titleLabel
            text: progressBar.title
            width: parent.width
            color: theme.highlightColor
            font.pixelSize: theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
        }
        Label {
            //% "Tap to cancel"
            text: progressBar.cancelText
            color: mouseArea.down ? theme.secondaryHighlightColor : theme.secondaryColor
            font.pixelSize: theme.fontSizeExtraSmall
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            progressBar.opacity = 0.0
            stopped()
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }
}
