/****************************************************************************
**
** Copyright (c) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativewebcontainer.h"

DeclarativeWebContainer::DeclarativeWebContainer(QObject *parent)
    : QObject(parent)
{
}

int DeclarativeWebContainer::findParentTabId(int) const
{
    return 0;
}
