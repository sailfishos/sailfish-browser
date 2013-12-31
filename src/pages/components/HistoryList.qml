/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/
import QtQuick 2.1
import Sailfish.Silica 1.0

SilicaListView {
    id: view
    property string search

    signal load(string url, string title)

    anchors.fill: parent

    // To prevent model to steal focus
    currentIndex: -1

    delegate: BackgroundItem {
        width: view.width
        height: Theme.itemSizeLarge

        Column {
            width: view.width - Theme.paddingLarge * 2
            x: Theme.paddingLarge
            anchors.verticalCenter: parent.verticalCenter

            Label {
                text: Theme.highlightText(title, search, Theme.highlightColor)
                truncationMode: TruncationMode.Fade
                width: parent.width
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
            }
            Label {
                text: Theme.highlightText(url, search, Theme.highlightColor)
                width: parent.width
                font.pixelSize: Theme.fontSizeSmall
                opacity: 0.6
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Elide
            }
        }

        onClicked: {
            Qt.inputMethod.hide()
            view.load(model.url, model.title)
        }
    }

    VerticalScrollDecorator {}
}
