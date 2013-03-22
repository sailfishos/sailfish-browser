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
        anchors.fill: parent
        color: theme.highlightDimmerColor
    }

    Rectangle {
        id: progressRect
        anchors {
            top: parent.top
            left: parent.left
        }
        height: parent.height
        width: opacity > 0.0 ? progressBar.progress * parent.width : 0
        color: theme.highlightBackgroundColor
        opacity: theme.highlightBackgroundOpacity

        Behavior on width {
            enabled: progressBar.opacity == 1.0
            SmoothedAnimation {
                velocity: 480; duration: 200
            }
        }
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
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
        }
        Label {
            //% "Tap to cancel"
            text: progressBar.cancelText
            anchors.horizontalCenter: parent.horizontalCenter
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

    Behavior on opacity { FadeAnimation {} }
}
