/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/
import QtQuick 1.1
import Sailfish.Silica 1.0

Image {
    property string favicon
    property string link

    source: favicon != "" ? favicon : WebUtils.getFaviconForUrl(link)
    height: theme.iconSizeSmall
    width: theme.iconSizeSmall
    asynchronous: true
    smooth: true
    fillMode: Image.PreserveAspectCrop
    clip: true

    onStatusChanged: {
        if (status == Image.Error) {
            source = "image://theme/icon-m-region"
        }
    }
}
