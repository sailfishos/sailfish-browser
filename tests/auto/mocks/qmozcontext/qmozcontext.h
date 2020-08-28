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

    explicit QMozContext(QObject *parent = 0) : QObject(parent) {};

    static QMozContext* instance();
    MOCK_METHOD2(setPref, void(const QString &, QVariant const &));
    MOCK_METHOD1(setPixelRatio, void(float));
    MOCK_CONST_METHOD0(initialized, bool());
    MOCK_CONST_METHOD0(pixelRatio, float());
    MOCK_METHOD1(CancelTask, void(void *));
    MOCK_METHOD2(PostCompositorTask, TaskHandle(TaskCallback, void *));
    MOCK_METHOD2(sendObserve, void(const QString &, const QString &));
    MOCK_METHOD2(sendObserve, void(const QString &, const QVariant &));

    MOCK_METHOD2(notifyObservers, void(const QString &, const QString &));
    MOCK_METHOD2(notifyObservers, void(const QString &, const QVariant &));

    MOCK_METHOD1(addObserver, void(const QString &));
    MOCK_METHOD1(addObservers, void(const std::vector<std::string> &));

signals:
    void onInitialized();
    void contextDestroyed();
    void lastViewDestroyed();
    void lastWindowDestroyed();
    void recvObserve(const QString, const QVariant);
};

#endif /* qmozcontext_h */
