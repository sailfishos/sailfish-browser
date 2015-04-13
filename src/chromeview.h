/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef CHROMEVIEW_H
#define CHROMEVIEW_H

#include <QQuickView>

class ChromeView : public QQuickView
{
    Q_OBJECT
public:
    ChromeView(QWindow *parent = 0);
};

#endif
