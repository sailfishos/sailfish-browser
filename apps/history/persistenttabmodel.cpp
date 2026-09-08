/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "persistenttabmodel.h"

PersistentTabModel::PersistentTabModel(int nextTabId, DeclarativeWebContainer *webContainer)
    : HostedTabModel(nextTabId, true, webContainer)
{
}

PersistentTabModel::~PersistentTabModel()
{
}
