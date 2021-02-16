/****************************************************************************
**
** Copyright (C) 2014
** Contact: Siteshwar Vashisht <siteshwar AT gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0

Dialog {
    id: configDialog

    property var changedConfigs: ({})

    // Get all the preferences
    Component.onCompleted: WebEngine.notifyObservers("embedui:allprefs", {})

    // If dialog is accepted, save all the changed configs
    onAccepted: {
        for (var key in changedConfigs) {
            WebEngineSettings.setPreference(key, changedConfigs[key]);
        }
        WebEngine.notifyObservers("embedui:saveprefs", {})
    }

    function filterModel(value) {
        if (value === "") {
            prefsList.model = prefsListModel
        } else {
            filterListModel.clear();
            for (var i=0; i<prefsListModel.count; i++) {
                if (prefsListModel.get(i).name.toLowerCase().search(value.toLowerCase()) != -1) {
                    var obj = prefsListModel.get(i);
                    obj.prefsListIndex = i;
                    filterListModel.append(obj);
                }
            }
            prefsList.model = filterListModel;
        }
    }

    Connections {
        target: WebEngine
        onRecvObserve: {
            if (message === "embed:allprefs") {
                var allprefs = data;
                prefsListModel.clear();
                for (var i=0; i < allprefs.length; i++) {
                    prefsListModel.append(allprefs[i]);
                }
            }
        }
    }

    ListModel {
        id: prefsListModel
    }

    ListModel {
        id: filterListModel
    }

    SilicaListView {
        id: prefsList
        model: prefsListModel
        width: parent.width
        height: parent.height

        VerticalScrollDecorator { flickable: prefsList }

        header: Column {
            DialogHeader {
                id: dialogHeader
                dialog: configDialog
                title: "about:config"
                _glassOnly: true
            }

            SearchField {
                width: prefsList.width
                //: Placeholder text for search (used in about:config page).
                //% "Search"
                placeholderText: qsTrId("sailfish_browser-ph-search")
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

                EnterKey.onClicked: filterModel(text)
            }
        }

        delegate: Loader {
            id: loader
            height: Theme.itemSizeMedium
            width: prefsList.width
            sourceComponent: model.type === 128 ? textSwitch : textField

            Component {
                id: textField

                TextField {
                    label: model.name
                    text: model.value
                    placeholderText: model.name
                    inputMethodHints: (model.type == 64 ? Qt.ImhDigitsOnly : 0)
                    width: parent.width
                    height: Theme.itemSizeMedium

                    onTextChanged: {
                        if (text === model.value) return;

                        if (model.type == 64) {
                            changedConfigs[model.name] = parseInt(text);
                        } else {
                            changedConfigs[model.name] =  text;
                        }

                        if (prefsList.model === prefsListModel) {
                            prefsListModel.setProperty(model.index, "value", text);
                        } else {
                            filterListModel.setProperty(model.index, "value", text);
                            prefsListModel.setProperty(model.prefsListIndex, "value", text);
                        }
                    }
                }
            }

            Component {
                id: textSwitch

                TextSwitch {
                    text: model.name
                    checked: model.value === "true"
                    width: parent.width
                    height: Theme.itemSizeMedium

                    onCheckedChanged: {
                        if (checked.toString() !== model.value) {
                            changedConfigs[model.name] = checked;

                            if (prefsList.model === prefsListModel) {
                                prefsListModel.setProperty(model.index, "value", checked.toString());
                            } else {
                                filterListModel.setProperty(model.index, "value", checked.toString());
                                prefsListModel.setProperty(model.prefsListIndex, "value", checked.toString());
                            }
                        }
                    }
                }
            }
        }
    }
}
