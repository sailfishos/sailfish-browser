/****************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BROWSER_PRIVATE_H
#define BROWSER_PRIVATE_H

class QQuickView;

class BrowserPrivate
{
    friend class Browser;

public:
    BrowserPrivate(QQuickView *view);

private:
    void initUserData();

protected:
    QQuickView *view;
};

#endif // BROWSER_PRIVATE_H
