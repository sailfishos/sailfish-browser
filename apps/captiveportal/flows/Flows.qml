/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
pragma Singleton
import QtQuick 2.6
import "." as Portal

QtObject {
    id: factory

    // Creation context must outlive a completed sign-on page. Parenting a
    // page-created Component to its window alone does not retain that context.
    property Component flowComponent: Component { Portal.PortalFlow {} }

    function create(owner, returnWindow) {
        return flowComponent.createObject(owner, {
            "returnWindow": returnWindow, "destroyOnClose": true
        })
    }
}
