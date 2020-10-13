import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

Page {
    id: root

    property alias model: view.model

    signal loadPage(string url)

    HistoryList {
        id: view

        anchors.fill: parent
        showDeleteButton: true

        onLoad: {
            view.focus = true
            pageStack.pop()
            root.loadPage(url)
        }

        header: Column {
            width: parent.width
            PageHeader {
                //% "History"
                title: qsTrId("sailfish_browser-he-history")
            }
            SearchField {
                id: searchField
                width: parent.width
                //% "Search"
                placeholderText: qsTrId("sailfish_browser-ph-search")
                EnterKey.onClicked: focus = false
                onTextChanged: {
                    model.search(searchField.text)
                    view.search = searchField.text
                }
            }
        }
        section {
            property: "date"
            delegate: SectionHeader {
                property string formattedDate: Format.formatDate(section, Formatter.TimepointSectionRelative)

                //% "Today"
                text: formattedDate ? formattedDate : qsTrId("sailfish_browser-la-today")
            }
        }

        ViewPlaceholder {
            //% "Websites you visit show up here"
            text: qsTrId("sailfish_browser-la-websites-show-up-here")
            enabled: !model.count
        }
    }
}
