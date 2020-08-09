import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView.Controls 1.0

Page {
    ListModel {
        id: permissionListModel
        ListElement {
            //% "Cookies"
            title: qsTrId("sailfish_browser-ti-cookies")
            type: "cookie"
        }
        ListElement {
            //% "Location"
            title: qsTrId("sailfish_browser-ti-location")
            type: "geolocation"
        }
        ListElement {
            //% "Popup windows"
            title: qsTrId("sailfish_browser-ti-popup")
            type: "popup"
        }
        ListElement {
            //% "Notifications"
            title: qsTrId("sailfish_browser-ti-notifications")
            type: "desktop-notification"
        }
    }

    PermissionModel {
        id: permissionModel
    }

    Timer {
        running: true
        interval: 5000
        onTriggered: permissionModel.add("https://yandex.ru", "geolocation", PermissionManager.Allow)
    }

    SilicaListView {
        id: listView
        header: PageHeader {
            //% "Permissions for sites"
            title: qsTrId("sailfish_browser-he-permissions_for_sites")
        }
        anchors.fill: parent
        model: permissionListModel
        delegate: BackgroundItem {
            width: listView.width
            Label {
                text: model.title
                anchors.verticalCenter: parent.verticalCenter
                x: Theme.horizontalPageMargin
            }
            onClicked: pageStack.animatorPush("PermissionListPage.qml", {
                                                  filterType: model.type,
                                                  title: model.title,
                                                  model: permissionModel
                                              })
        }
    }
}
