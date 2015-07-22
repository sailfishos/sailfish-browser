/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEWEBCONTAINER_H
#define DECLARATIVEWEBCONTAINER_H

#include <QObject>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

class DeclarativeWebPage;

class DeclarativeWebContainer : public QObject
{
    Q_OBJECT

public:
    explicit DeclarativeWebContainer(QObject *parent = 0);

    int findParentTabId(int) const;
    MOCK_CONST_METHOD0(webPage, DeclarativeWebPage*());
};


#endif
