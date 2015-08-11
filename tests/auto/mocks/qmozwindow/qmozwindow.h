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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

class QMozWindow : public QObject
{
    Q_OBJECT

public:
    explicit QMozWindow(QObject *parent = 0) : QObject(parent) {};

    MOCK_METHOD1(setContentOrientation, void(Qt::ScreenOrientation));
    MOCK_METHOD1(setSize, void(QSize));

signals:
    void requestGLContext();
    void initialized();
    void drawOverlay(QRect);
    void drawUnderlay();
    void compositorCreated();
    void compositingFinished();
};

#endif /* qmozwindow_h */
