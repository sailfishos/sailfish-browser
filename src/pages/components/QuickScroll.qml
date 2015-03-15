/****************************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Joona Petrell <joona.petrell@jollamobile.com>
** All rights reserved.
** 
** This file is part of Sailfish Silica UI component package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Jolla Ltd nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

import QtQuick 2.1
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

Item {
    property WebPage flickable
    //property bool quickScroll: flickable && (flickable.flickableDirection === Flickable.VerticalFlick || flickable.flickableDirection === Flickable.AutoFlickDirection)
    property bool quickScroll: flickable
    property bool quickScrollAnimating
    property bool _quickScrollAllowed: _initialised && quickScroll && flickable.height >= Screen.width && flickable.contentHeight > 3.5*flickable.height
    property Item _quickScrollArea
    property bool _incubating
    property bool _initialised
    property bool _moving: flickable && flickable.moving
    property var _incubator

    Component.onCompleted: _initialised = true
    /*on_QuickScrollAllowedChanged: {
        if (_quickScrollAllowed) {
            if (!_quickScrollArea && !_incubating) {
                _incubator = quickScrollAreaComponent.incubateObject(flickable, {"flickable": flickable })
                if (_incubator.status != Component.Ready) {
                    _incubating = true
                    _incubator.onStatusChanged = function(status) {
                        if (!quickScroll) {
                            _quickScrollArea.destroy()
                            _quickScrollArea = null
                        }
                        if (status == Component.Ready) {
                            _quickScrollArea = _incubator.object
                            _incubating = false
                        } else if (status == Component.Error) {
                            _incubating = false
                        }
                    }
                } else {
                    _quickScrollArea = _incubator.object
                }
            }
        }
    }*/

    // See bug #21387
    /*on_MovingChanged: {
        if (_moving && _quickScrollAllowed && !_quickScrollArea && _incubator.status != Component.Ready) {
            _incubator.forceCompletion()
        }
    }*/

    onQuickScrollChanged: {
        if (!quickScroll && _quickScrollArea) {
            _quickScrollArea.destroy()
            _quickScrollArea = null
        }
    }
    Binding {
        when: _quickScrollArea && !_quickScrollAllowed
        target: _quickScrollArea
        property: "active"
        value: false
    }
    Component {
        id: quickScrollAreaComponent
        QuickScrollArea {}
    }
}
