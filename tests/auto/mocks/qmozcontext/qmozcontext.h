/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozcontext_h
#define qmozcontext_h

#include <QObject>
#include <QVariant>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

class QMozContext : public QObject
{
    Q_OBJECT

public:
    typedef void (*TaskCallback)(void* data);
    typedef void* TaskHandle;

    explicit QMozContext(QObject *parent = nullptr) : QObject(parent) {}

    static QMozContext* instance();

    MOCK_METHOD(void, setPref, (const QString &, QVariant const &));
    MOCK_METHOD(float, setPixelRatio, (float));
    MOCK_METHOD(bool, isInitialized, (), (const));
    MOCK_METHOD(float, pixelRatio, (), (const));
    MOCK_METHOD(void, CancelTask, (void *));
    MOCK_METHOD(TaskHandle, PostCompositorTask, (TaskCallback, void *));
    MOCK_METHOD(void, sendObserve, (const QString &, const QString &));
    MOCK_METHOD(void, sendObserve, (const QString &, const QVariant &));

    MOCK_METHOD(void, notifyObservers, (const QString &, const QString &));
    MOCK_METHOD(void, notifyObservers, (const QString &, const QVariant &));

    MOCK_METHOD(void, addObserver, (const QString &));
    MOCK_METHOD(void, addObservers, (const std::vector<std::string> &));

signals:
    void initialized();
    void contextDestroyed();
    void lastWindowDestroyed();
    void recvObserve(const QString, const QVariant);
};

#endif /* qmozcontext_h */
