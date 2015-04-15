/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef INPUTREGION_PRIVATE_H
#define INPUTREGION_PRIVATE_H

#include <QObject>
#include <QWindow>
#include "inputregion.h"

class InputRegionPrivate {
public:
    InputRegionPrivate(InputRegion *q);

    void update();

    qreal x;
    qreal y;
    qreal width;
    qreal height;
    QWindow *window;
    InputRegion *q_ptr;

    Q_DECLARE_PUBLIC(InputRegion)
};

#endif
