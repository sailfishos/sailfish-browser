/****************************************************************************
**
** Copyright (c) 2014
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
    property bool prefsLoaded
    property bool allPrefsObserverAdded
    property int allPrefsRequests
    property string searchText
    property bool modifiedOnly: false
    property var sourcePrefs: []
    property var pendingPrefs: []
    property int pendingPrefsIndex
    readonly property int minimumSearchLength: 3
    readonly property int prefsAppendBatchSize: 80
    readonly property real preferenceMainFontSize: Theme.fontSizeMedium
    readonly property real preferenceNameFontSize: preferenceMainFontSize
    readonly property real preferenceValueFontSize: preferenceMainFontSize
    readonly property real preferenceSecondaryFontSize: Theme.fontSizeSmall
    readonly property real preferenceMinimumHeight: Math.round((Theme.itemSizeMedium + Theme.itemSizeLarge) / 2) + Theme.paddingLarge

    // Get all the preferences
    Component.onCompleted: resetAndRequestAllPrefs()

    Component.onDestruction: {
        saveChangedPrefs(false)
        if (allPrefsObserverAdded) {
            WebEngine.removeObserver("embed:allprefs")
        }
    }

    // If dialog is accepted, save all the changed configs
    onAccepted: saveChangedPrefs(true)

    function saveChangedPrefs(forceSave) {
        var changedPrefs = []
        for (var key in changedConfigs) {
            var preference = changedConfigs[key]
            changedPrefs.push({
                name: key,
                value: preference.value
            })
        }
        if (changedPrefs.length > 0) {
            WebEngine.notifyObservers("embedui:setprefs", { prefs: changedPrefs })
            changedConfigs = ({})
        }
        if (forceSave || changedPrefs.length > 0) {
            WebEngine.notifyObservers("embedui:saveprefs", {})
        }
    }

    function createPreferenceItem(preference, sourceIndex) {
        return {
            name: preference.name,
            value: preference.value,
            modified: preference.modified,
            lock: preference.lock,
            type: preference.type,
            searchName: preference.name.toLowerCase(),
            prefsListIndex: sourceIndex
        }
    }

    function updateSearchFilter(value, resetForShortSearch) {
        var filter = (value || "").toLowerCase()
        if (filter.length < minimumSearchLength) {
            if (filter.length === 0 || resetForShortSearch) {
                resetDisplayedPrefs(sourcePrefs)
            }
            return
        }

        var filteredPrefs = []
        for (var i=0; i<sourcePrefs.length; i++) {
            const pref = sourcePrefs[i]
            if (modifiedOnly && !pref.modified) continue
            if (pref.searchName.indexOf(filter) != -1) {
                filteredPrefs.push(pref)
            }
        }
        resetDisplayedPrefs(filteredPrefs)
    }

    function resetDisplayedPrefs(preferences) {
        pendingPrefsAppend.stop()
        pendingPrefs = preferences || []
        pendingPrefsIndex = 0
        prefsListModel.clear()
        appendPendingPrefs()
    }

    function appendPendingPrefs() {
        if (pendingPrefsIndex >= pendingPrefs.length) {
            pendingPrefsAppend.stop()
            return
        }

        var end = Math.min(pendingPrefsIndex + prefsAppendBatchSize, pendingPrefs.length)
        for (var i=pendingPrefsIndex; i<end; i++) {
            prefsListModel.append(pendingPrefs[i])
        }
        pendingPrefsIndex = end

        if (pendingPrefsIndex < pendingPrefs.length) {
            pendingPrefsAppend.restart()
        }
    }

    function requestAllPrefs() {
        if (!WebEngine.initialized) {
            if (allPrefsRequests < 6 && !allPrefsRetry.running) {
                allPrefsRetry.start()
            }
            return
        }

        if (!allPrefsObserverAdded) {
            WebEngine.addObserver("embed:allprefs")
            allPrefsObserverAdded = true
        }

        ++allPrefsRequests
        WebEngine.notifyObservers("embedui:allprefs", {})
        if (!prefsLoaded && allPrefsRequests < 6 && !allPrefsRetry.running) {
            allPrefsRetry.start()
        }
    }

    function resetAndRequestAllPrefs() {
        prefsLoaded = false
        allPrefsRequests = 0
        requestAllPrefs()
    }

    function updatePreferenceValue(modelIndex, prefsListIndex, name, value, type) {
        changedConfigs[name] = {
            value: type == WebEngineSettings.IntPref ? parseInt(value, 10) : value,
            type: type
        }

        var modelValue = value.toString()
        prefsListModel.setProperty(modelIndex, "value", modelValue)
        sourcePrefs[prefsListIndex].value = modelValue
    }

    Connections {
        target: WebEngine
        onInitialized: requestAllPrefs()
        onRecvObserve: {
            if (message === "embed:allprefs") {
                var allprefs = data
                var preferences = []
                for (var i=0; i < allprefs.length; i++) {
                    preferences.push(createPreferenceItem(allprefs[i], i))
                }
                sourcePrefs = preferences
                searchFilterDelay.stop()
                updateSearchFilter(searchText, true)
                if (allprefs.length > 0) {
                    prefsLoaded = true
                    allPrefsRetry.stop()
                }
            }
        }
    }

    Timer {
        id: allPrefsRetry

        interval: 500
        repeat: true
        onTriggered: {
            if (prefsLoaded || allPrefsRequests >= 6) {
                stop()
            } else {
                requestAllPrefs()
            }
        }
    }

    Timer {
        id: searchFilterDelay

        interval: 250
        onTriggered: updateSearchFilter(configDialog.searchText)
    }

    Timer {
        id: pendingPrefsAppend

        interval: 0
        onTriggered: appendPendingPrefs()
    }

    ListModel {
        id: prefsListModel
    }

    Column {
        anchors.fill: parent

        DialogHeader {
            id: dialogHeader

            width: parent.width
            dialog: configDialog
            title: "about:config"
            _glassOnly: true
        }

        SearchField {
            id: searchField

            width: parent.width
            //: Placeholder text for search (used in about:config page).
            //% "Search"
            placeholderText: qsTrId("sailfish_browser-ph-search")
            font.pixelSize: preferenceValueFontSize
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

            onTextChanged: {
                configDialog.searchText = text
                searchFilterDelay.restart()
            }
            EnterKey.onClicked: {
                focus = false
            }
        }

        TextSwitch {
            checked: configDialog.modifiedOnly
            // % "Show only modified Preferences"
            text: qsTrId("sailfish_browser-ph-search_only_modified")
            onClicked: {
                configDialog.modifiedOnly = !configDialog.modifiedOnly
                searchFilterDelay.restart()
            }
        }

        SilicaListView {
            id: prefsList

            model: prefsListModel
            width: parent.width
            height: Math.max(0, parent.height - dialogHeader.height - searchField.height)
            clip: true

            VerticalScrollDecorator { flickable: prefsList }

            delegate: Loader {
                active: true
                visible: true
                height: item ? item.height : preferenceMinimumHeight
                width: prefsList.width
                sourceComponent: model.type == WebEngineSettings.BoolPref ? textSwitch : textField

                Component {
                    id: textField

                    Item {
                        width: prefsList.width
                        height: Math.max(preferenceMinimumHeight,
                                         textColumn.height + 2 * Theme.paddingMedium)

                        Column {
                            id: textColumn

                            x: Theme.horizontalPageMargin
                            y: Theme.paddingMedium
                            width: parent.width - 2 * x
                            spacing: Theme.paddingMedium

                            Label {
                                width: parent.width
                                color: Theme.primaryColor
                                font.pixelSize: preferenceNameFontSize
                                wrapMode: Text.Wrap
                                text: model.name
                            }

                            TextField {
                                readonly property bool digitsOnly: model.type == WebEngineSettings.IntPref

                                label: {
                                    if (errorHighlight && digitsOnly) {
                                        //% "Please enter only integer values"
                                        return qsTrId("sailfish_browser-la-only_integer_values")
                                    }
                                    return ""
                                }
                                text: model.value
                                placeholderText: model.name
                                font.pixelSize: preferenceValueFontSize
                                inputMethodHints: digitsOnly ? Qt.ImhDigitsOnly : 0
                                width: parent.width

                                Component.onCompleted: {
                                    if (digitsOnly) {
                                        validator = intValidator
                                    }
                                }

                                onTextChanged: {
                                    if (text === model.value)
                                        return

                                    configDialog.updatePreferenceValue(model.index, model.prefsListIndex,
                                                                       model.name, text, model.type)
                                }
                            }
                        }

                        IntValidator { id: intValidator }
                    }
                }

                Component {
                    id: textSwitch

                    BackgroundItem {
                        id: boolItem

                        readonly property bool prefChecked: model.value === "true"

                        width: prefsList.width
                        height: Math.max(preferenceMinimumHeight,
                                         boolColumn.height + 2 * Theme.paddingMedium)

                        onClicked: {
                            configDialog.updatePreferenceValue(model.index, model.prefsListIndex,
                                                               model.name, !prefChecked, model.type)
                        }

                        Column {
                            id: boolColumn

                            anchors {
                                left: parent.left
                                leftMargin: Theme.horizontalPageMargin
                                right: toggle.left
                                rightMargin: Theme.paddingMedium
                                verticalCenter: parent.verticalCenter
                            }
                            spacing: Theme.paddingMedium

                            Label {
                                width: parent.width
                                color: boolItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                                font.pixelSize: preferenceNameFontSize
                                wrapMode: Text.Wrap
                                text: model.name
                            }

                            Label {
                                width: parent.width
                                color: boolItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                                font.pixelSize: preferenceSecondaryFontSize
                                text: boolItem.prefChecked ? "true" : "false"
                            }
                        }

                        Switch {
                            id: toggle

                            anchors {
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }
                            checked: boolItem.prefChecked

                            onClicked: boolItem.clicked(mouse)
                        }
                    }
                }
            }
        }
    }
}
