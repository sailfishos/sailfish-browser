/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelaine@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBENGINE_H
#define WEBENGINE_H

#include <QObject>
#include <QVariant>
#include <vector>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace SailfishOS {

class WebEngine : public QObject
{
    Q_OBJECT

public:
    typedef void (*TaskCallback)(void* data);
    typedef void* TaskHandle;

    explicit WebEngine(QObject *parent = 0) : QObject(parent) {};

    static void initialize(const QString &profilePath, const QString &userAgent = QLatin1String(""));
    static WebEngine* instance();
    MOCK_CONST_METHOD0(initialized, bool());
    MOCK_METHOD1(CancelTask, void(void *));
    MOCK_METHOD2(PostCompositorTask, TaskHandle(TaskCallback, void *));
    MOCK_METHOD2(sendObserve, void(const QString &, const QString &));
    MOCK_METHOD2(sendObserve, void(const QString &, const QVariant &));

    MOCK_METHOD2(notifyObservers, void(const QString &, const QString &));
    MOCK_METHOD2(notifyObservers, void(const QString &, const QVariant &));

    MOCK_METHOD1(addObserver, void(const std::string &));
    MOCK_METHOD1(addObservers, void(const std::vector<std::string> &));

signals:
    void onInitialized();
    void contextDestroyed();
    void lastViewDestroyed();
    void lastWindowDestroyed();
    void recvObserve(const QString, const QVariant);
};
}

#endif // WEBENGINE_H
