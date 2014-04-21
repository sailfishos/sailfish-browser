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

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QQuickItem>
#include <QSignalSpy>
#include <QtTest>

TestObject::TestObject()
    : QObject()
{
}

TestObject::TestObject(QByteArray qmlData)
    : QObject()
{
    setTestData(qmlData);
    mView.show();
    QTest::qWaitForWindowExposed(&mView);
}

void TestObject::init(const QUrl &url)
{
    setTestUrl(url);
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
    int i = 0;
    int maxWaits = 10;
    while (spy.count() < expectedSignalCount && i < maxWaits) {
        spy.wait();
        ++i;
    }
}

void TestObject::setTestData(QByteArray qmlData)
{
    QQmlComponent component(mView.engine());
    component.setData(qmlData, QUrl());
    mRootObject = component.create(mView.engine()->rootContext());
    mView.setContent(QUrl(""), 0, mRootObject);
}

void TestObject::setTestUrl(const QUrl &url)
{
    mView.setSource(url);
    mRootObject = mView.rootObject();
}

void TestObject::setContextProperty(const QString &name, QObject *value)
{
    mView.rootContext()->setContextProperty(name, value);
}
