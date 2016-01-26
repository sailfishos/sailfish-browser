/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QQmlComponent>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "tab.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"

class WebPageFactory : public QObject
{
    Q_OBJECT
public:
    WebPageFactory(QObject *parent = 0) : QObject(parent) {};

    MOCK_METHOD3(createWebPage, DeclarativeWebPage*(DeclarativeWebContainer*, const Tab&, int));

public slots:
    void updateQmlComponent(QQmlComponent*) {};
};
