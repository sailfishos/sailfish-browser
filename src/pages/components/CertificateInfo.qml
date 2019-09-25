/*
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as Private
import Sailfish.Browser 1.0
import Qt5Mozilla 1.0
import "." as Browser

SilicaFlickable {
    id: root
    property QMozSecurity security
    readonly property bool _validCert: security && security.subjectDisplayName.length > 0
    contentHeight: certInfoColumn.height + certInfoColumn.y + showMore.height + calculatedHeight + 2 * Theme.paddingSmall
    clip: contentHeight > height
    property alias buttonHeight: showMore.height
    readonly property bool _secure: security && security.allGood
    property bool portrait
    property bool initialized
    property int implicitKeyWidth: Theme.itemSizeExtraLarge
    property int keyWidth: implicitKeyWidth
    property int maximumKeyWidth: width / 2
    property int calculatedHeight

    signal showCertDetail
    signal closeCertInfo

    function updateKeyWidth() {
        if (!initialized) return

        var width = implicitKeyWidth
        for (var i in keyValuePairs.children) {
            var child = keyValuePairs.children[i]
            if (child.keyLabel !== undefined) {
                var childWidth = Math.ceil(child.keyLabel.implicitWidth)
                if (childWidth > maximumKeyWidth) continue
                width = Math.max(width, childWidth)
            }
        }
        keyWidth = width

        // Column won't layout until the next frame, so we have to calculate the height ourselves
        calculatedHeight = 0
        for (var i in keyValuePairs.children) {
            var child = keyValuePairs.children[i]
            if ((child.keyLabel !== undefined) && (child.value.length > 0)) {
                calculatedHeight += child.height + Theme.paddingSmall
            }
        }
        if (calculatedHeight > 0) {
            calculatedHeight -= Theme.paddingSmall
        }
    }

    Component.onCompleted: {
        initialized = true // avoid unnecessary re-layouting during initialization
        updateKeyWidth()
    }

    onSecurityChanged: {
        // Jump back to the top
        contentY = originY
    }

    onMaximumKeyWidthChanged: updateKeyWidth()

    VerticalScrollDecorator {}

    MouseArea {
        anchors.fill: parent
        onClicked: closeCertInfo()
    }

    Column {
        id: certInfoColumn
        spacing: Theme.paddingSmall
        width: parent.width
        y: Theme.paddingMedium
        bottomPadding: showMore.enabled ? 0 : Theme.paddingLarge

        Label {
            width: parent.width - 2 * Theme.horizontalPageMargin
            x: Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeLarge
            color: _secure ? (palette.colorScheme === Theme.LightOnDark ? "#22ff22" : "#007700")
                           : Theme.errorColor
            text: {
                if (_secure) {
                    //: The SSL/TLS connection is good
                    //% "Connection is secure"
                    return qsTrId("sailfish_browser-la-cert_connection_secure")
                }
                //: Either no SSL/TLS is in use, or the SSL/TLS connection is broken in some way
                //% "Connection is not secure"
                return qsTrId("sailfish_browser-la-cert_connection_insecure")
            }
        }

        Label {
            width: parent.width - 2 * Theme.horizontalPageMargin
            x: Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            visible: text.length > 0

            text: {
                if (security) {
                    if (security.certIsNull) {
                        //% "The page does't offer any security"
                        return qsTrId("sailfish_browser-la-cert_none")
                    } else if (security.domainMismatch) {
                        // "The website doesn't match the name on the security certificate"
                        //: The domain in the TLS certificate and the domain connected to are not the same
                        //% "This site may not be legitimate and any information you enter could be stolen by attackers"
                        return qsTrId("sailfish_browser-la-cert_domain_mismatch")
                    } else if (security.notValidAtThisTime) {
                        //: The TLS certificate has expired or is not yet valid
                        //% "The security certificate has expired or isn't valid yet"
                        return qsTrId("sailfish_browser-la-cert_not_valid_at_this_time")
                    } else if (security.usesWeakCrypto) {
                        //: The TLS certificate is insecure because it uses weak encryption or hashing methods
                        //% "The conection only provides weak security"
                        return qsTrId("sailfish_browser-la-cert_weak_crypto")
                    } else if (security.loadedMixedActiveContent) {
                        //% "Some content on the page is transmitted insecurely"
                        return qsTrId("sailfish_browser-la-cert_insecure_content_loaded")
                    } else if (security.loadedMixedDisplayContent) {
                        // Display the same message as for mixed content
                        return qsTrId("sailfish_browser-la-cert_insecure_content_loaded")
                    } else if (security.blockedMixedActiveContent) {
                        //% "Parts of the page that are not secure have been blocked"
                        return qsTrId("sailfish_browser-la-cert_content_active_blocked")
                    } else if (security.blockedMixedDisplayContent) {
                        //% "Parts of the page that are not secure have been blocked"
                        return qsTrId("sailfish_browser-la-cert_content_display_blocked")
                    }
                }
                return ""
            }
            color: Theme.errorColor
            opacity: Theme.opacityHigh
        }

        SectionHeader {
            //: Header separating out the main security info from the details
            //% "Certificate"
            text: qsTrId("sailfish_browser-sh-cert_details")
            visible: security && !security.certIsNull
        }
    }

    Column {
        id: keyValuePairs
        width: parent.width
        spacing: Theme.paddingSmall
        anchors {
            top: certInfoColumn.bottom
            topMargin: Theme.paddingSmall
        }

        onHeightChanged: calculatedHeight = height

        CertificateLabel {
            //: Label for the owner field of a TLS certificate
            //% "Owner"
            key: qsTrId("sailfish_browser-la-cert_owner")
            value: security ? security.subjectOrganization : ""
            valueColor: _secure ? palette.colorScheme === Theme.LightOnDark
                                  ? "#22ff22" : "#007700" : Theme.secondaryHighlightColor
            keyWidth: root.keyWidth
        }

        CertificateLabel {
            //: Label for the subject field of a TLS certificate
            //% "Website"
            key: qsTrId("sailfish_browser-la-cert_subject")
            value: security ? security.subjectDisplayName : ""
            keyWidth: root.keyWidth
        }

        CertificateLabel {
            //: Label for the issuer field of a TLS certificate
            //% "Issuer"
            key: qsTrId("sailfish_browser-la-cert_issuer")
            value: security ? security.issuerDisplayName : ""
            keyWidth: root.keyWidth
        }

        CertificateLabel {
            //: Label for the TLS certificate validity period (dates)
            //% "Valid"
            key: qsTrId("sailfish_browser-la-cert_valid_dates")
            value: insecure ? Format.formatDate(security.effectiveDate, Formatter.DateMedium)
                            + " — "
                            + Format.formatDate(security.expiryDate, Formatter.DateMedium)
                          : ""
            insecure: security && !security.certIsNull && security.notValidAtThisTime
            keyWidth: root.keyWidth
        }

        CertificateLabel {
            //: Label prefixing the status of whether tracking tech is being used on a page
            //% "Tracking"
            key: qsTrId("sailfish_browser-la-cert_tracking")
            value: {
                if (security) {
                    if (security.loadedTrackingContent) {
                        //% "The page includes items used to track your behaviour"
                        return qsTrId("sailfish_browser-la-cert_tracking_content_loaded")
                    } else if (security.blockedTrackingContent) {
                        //% "Items used to track your behaviour have been blocked"
                        return qsTrId("sailfish_browser-la-cert_tracking_content_blocked")
                    }
                }
                return ""
            }
            insecure: security && security.loadedTrackingContent
            keyWidth: root.keyWidth
        }
    }

    BackgroundItem {
        id: showMore
        height: enabled ? portrait ? Theme.itemSizeSmall : Theme.itemSizeExtraSmall : 0
        enabled: security && !security.certIsNull
        visible: enabled
        anchors {
            top: keyValuePairs.bottom
            topMargin: Theme.paddingSmall
        }

        onClicked: showCertDetail()

        Private.ShowMoreButton {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            anchors.verticalCenter: parent.verticalCenter
            highlighted: showMore.highlighted
            enabled: false
        }
    }
}
