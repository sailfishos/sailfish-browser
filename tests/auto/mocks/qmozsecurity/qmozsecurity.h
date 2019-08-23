/*
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef qmozsecurity_h
#define qmozsecurity_h

#include <QObject>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

class QMozSecurity : public QObject
{
    Q_OBJECT

public:
    explicit QMozSecurity(QObject *parent = 0) : QObject(parent) {
    }

};

#endif /* qmozsecurity_h */

