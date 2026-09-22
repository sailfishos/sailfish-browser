/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

.pragma library

var Phase = Object.freeze({
    Idle: 0,
    AwaitingClose: 1,
    Dragging: 2,
    SettlingBack: 3,
    SettlingForward: 4,
    WaitingSelection: 5,
    WaitingFrame: 6,
    Fading: 7
})

var Operation = Object.freeze({
    None: 0,
    Swipe: 1,
    Close: 2
})
