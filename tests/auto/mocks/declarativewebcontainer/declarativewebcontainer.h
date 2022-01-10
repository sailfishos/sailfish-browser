/*
 * Copyright (c) 2015 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DECLARATIVEWEBCONTAINER_H
#define DECLARATIVEWEBCONTAINER_H

#include <QObject>
#include <qmozsecurity.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

class DeclarativeWebPage;

class DeclarativeWebContainer : public QObject
{
    Q_OBJECT

public:
    explicit DeclarativeWebContainer(QObject *parent = 0);

    int findTabId(uint32_t uniqueId) const;
    MOCK_CONST_METHOD0(webPage, DeclarativeWebPage*());
    MOCK_CONST_METHOD0(privateMode, bool());

signals:
    void portraitChanged();
};


#endif
