/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Piotr Tworek <piotr.tworek@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef qmozwindow_h
#define qmozwindow_h

#include <QObject>
#include <QSize>
#include <QRect>

#include <functional>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

class QMozWindowListener;

enum class QMozTextureTarget {
    Texture2D,
    ExternalOES
};

struct QMozEGLImage final
{
    EGLImageKHR image;
    QSize size;
    QMozTextureTarget textureTarget;
};

using QMozEGLImageCallback = std::function<void(const QMozEGLImage &)>;

class QMozWindow : public QObject
{
    Q_OBJECT

public:
    explicit QMozWindow(const QSize &size, QObject *parent = nullptr)
        : QObject(parent)
    {
        Q_UNUSED(size);
    }

    MOCK_METHOD(void, setSize, (QSize));
    MOCK_METHOD(QSize, size, (void));
    MOCK_METHOD(void, setContentOrientation, (Qt::ScreenOrientation));
    MOCK_METHOD(void, setPrimaryOrientation, (Qt::ScreenOrientation));
    MOCK_METHOD(Qt::ScreenOrientation, contentOrientation, ());
    MOCK_METHOD(Qt::ScreenOrientation, pendingOrientation, ());
    MOCK_METHOD(bool, withPlatformImage, (const QMozEGLImageCallback &));
    MOCK_METHOD(void, clearPlatformImage, (void));
    MOCK_METHOD(void, reserve, (void));
    MOCK_METHOD(void, release, (void));
    MOCK_METHOD(bool, isReserved, (void));
    MOCK_METHOD(void, suspendRendering, (void));
    MOCK_METHOD(void, resumeRendering, (void));
    MOCK_METHOD(void, scheduleUpdate, (void));
    MOCK_METHOD(bool, readyToPaint, (void));
    MOCK_METHOD(bool, setReadyToPaint, (bool));

signals:
    void pendingOrientationChanged(Qt::ScreenOrientation orientation);
    void orientationChangeFiltered(Qt::ScreenOrientation orientation);
    void initialized();
    void released();
    void drawOverlay(QRect);
    void compositorCreated();
    void compositingFinished();
};

#endif /* qmozwindow_h */
