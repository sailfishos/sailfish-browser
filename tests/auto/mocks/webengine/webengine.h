/****************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
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

    explicit WebEngine(QObject *parent = nullptr) : QObject(parent) {}

    static void initialize(const QString &profilePath);
    static WebEngine* instance();

    MOCK_METHOD(bool, isInitialized, (), (const));
    MOCK_METHOD(bool, stopEmbedding, (), (const));

    MOCK_METHOD(void, CancelTask, (void *));
    MOCK_METHOD(TaskHandle, PostCompositorTask, (TaskCallback, void *));
    MOCK_METHOD(void, sendObserve, (const QString &, const QString &));
    MOCK_METHOD(void, sendObserve, (const QString &, const QVariant &));

    MOCK_METHOD(void, notifyObservers, (const QString &, const QString &));
    MOCK_METHOD(void, notifyObservers, (const QString &, const QVariant &));

    MOCK_METHOD(void, addObserver, (const std::string &));
    MOCK_METHOD(void, addObservers, (const std::vector<std::string> &));

signals:
    void initialized();
    void contextDestroyed();
    void lastWindowDestroyed();
    void recvObserve(const QString, const QVariant);
};
}

#endif // WEBENGINE_H
