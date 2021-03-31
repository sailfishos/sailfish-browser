/****************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelaine@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBENGINE_SETTINGS_H
#define WEBENGINE_SETTINGS_H

#include <QObject>
#include <QVariant>
#include <QSize>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace SailfishOS {

class WebEngineSettings : public QObject
{
    Q_OBJECT

public:
    static WebEngineSettings *instance();
    static void initialize();

    explicit WebEngineSettings(QObject *parent = 0) : QObject(parent) {}

    MOCK_CONST_METHOD0(isInitialized, bool());

    MOCK_CONST_METHOD0(autoLoadImages, bool());
    MOCK_METHOD1(setAutoLoadImages, void(bool));

    MOCK_CONST_METHOD0(javascriptEnabled, bool());
    MOCK_METHOD1(setJavascriptEnabled, void(bool));

    MOCK_METHOD1(setTileSize, void(const QSize &));

    MOCK_CONST_METHOD0(pixelRatio, qreal());
    MOCK_METHOD1(setPixelRatio, void(qreal));

    MOCK_METHOD1(enableProgressivePainting, void(bool));
    MOCK_METHOD1(enableLowPrecisionBuffers, void(bool));

    MOCK_METHOD2(setPreference, void(const QString &, const QVariant &));

signals:
    void autoLoadImagesChanged();
    void javascriptEnabledChanged();
    void initialized();
};

}

#endif // WEBENGINE_SETTINGS_H
