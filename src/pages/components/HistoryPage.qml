import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

Page {
    HistoryList {
        id: view

        anchors.fill: parent

        model: HistoryModel { id: historyModel }

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
                    historyModel.search(searchField.text)
                    view.search = searchField.text
                }
            }
        }
        section {
            property: "date"
            delegate: SectionHeader {
                text: Format.formatDate(section, Formatter.TimepointSectionRelative)
            }
        }

        delegate: HistoryItem { search: view.search }

        ViewPlaceholder {
            //% "Websites you visit show up here"
            text: qsTrId("sailfish_browser-la-websites-show-up-here")
            enabled: !historyModel.count
        }
    }
}
