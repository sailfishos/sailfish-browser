/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0
import "pages"
import "cover"

ApplicationWindow
{
    id: window
    initialPage: Component {BrowserPage {}}
    cover: CoverPage {}
}

