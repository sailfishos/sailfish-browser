/****************************************************************************
**
** Copyright (c) 2015 Jolla Ltd.
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "privatetabmodel.h"

PrivateTabModel::PrivateTabModel(int nextTabId, DeclarativeWebContainer *webContainer)
    : HostedTabModel(nextTabId, false, webContainer)
{
}

PrivateTabModel::~PrivateTabModel()
{
}
