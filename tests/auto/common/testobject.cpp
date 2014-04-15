/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testobject.h"
#include "declarativehistorymodel.h"
#include "declarativetabmodel.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>
#include <QSignalSpy>
#include <QtTest>

TestObject::TestObject(QByteArray qmlData)
{
    setTestData(qmlData);
    mView.show();
    QTest::qWaitForWindowExposed(&mView);
}

/*!
    Wait signal of \a spy to be emitted \a expectedSignalCount.

    Note: this might cause indefinite loop, if not used cautiously. Check
    that \a spy is initialized before expected emits can happen.
*/
void TestObject::waitSignals(QSignalSpy &spy, int expectedSignalCount) const
{
    while (spy.count() < expectedSignalCount) {
        spy.wait();
    }
}

void TestObject::setTestData(QByteArray qmlData)
{
    QQmlComponent component(mView.engine());
    component.setData(qmlData, QUrl());
    mRootObject = component.create(mView.engine()->rootContext());
    mView.setContent(QUrl(""), 0, mRootObject);
}
