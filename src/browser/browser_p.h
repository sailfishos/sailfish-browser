/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BROWSER_PRIVATE_H
#define BROWSER_PRIVATE_H

class CloseEventFilter;
class QQuickView;

class BrowserPrivate {
public:
    BrowserPrivate(QQuickView *view);

    QQuickView *view;
    CloseEventFilter *closeEventFilter;
};

#endif // BROWSER_PRIVATE_H
