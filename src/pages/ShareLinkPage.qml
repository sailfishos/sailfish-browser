/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Antti Seppälä <antti.seppala@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.TransferEngine 1.0
import com.jolla.settings.accounts 1.0

Page {
    id: page

    property string link
    property string linkTitle

    ShareMethodList {
        id: shareMethodList
        anchors.fill: parent  
        header: PageHeader {
            //: List header for link sharing method list
            //% "Share link"
            title: qsTrId("sailfish_browser-he-share_link")
        }
        filter: "text/x-url"
        content: {
            "type": "text/x-url",
            "status": page.link,
            "linkTitle": page.linkTitle
        }

        // Add "add account" to the footer. User must be able to
        // create accounts in a case there are none.
        footer: BackgroundItem {
            Label {
                //: Add new account
                //% "Add account"
                text: qsTrId("sailfish_browser-la-add_account")
                anchors {
                    left: parent.left; leftMargin: Theme.paddingLarge
                    right: parent.right; rightMargin: Theme.paddingLarge
                    verticalCenter: parent.verticalCenter
                }
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            onClicked: {
                if (typeof jolla_signon_ui_service !== "undefined") {
                    jolla_signon_ui_service.inProcessParent = page
                }
                pageStack.push(accountsPage)
            }
        }

        Component {
            id: accountsPage
            AccountsPage { }
        }
    }
}
