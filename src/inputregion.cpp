/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "inputregion.h"
#include "inputregion_p.h"

#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

InputRegionPrivate::InputRegionPrivate(InputRegion *q)
    : x(0.0)
    , y(0.0)
    , width(0.0)
    , height(0.0)
    , window(0)
    , q_ptr(q)
{
}

void InputRegionPrivate::update()
{
    if (window && window->handle() && width > 0.0 && height > 0.0) {
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        native->setWindowProperty(window->handle(), QLatin1String("MOUSE_REGION"), QVariant(QRegion(x, y, width, height)));
    }
}


InputRegion::InputRegion(QObject *parent)
    : QObject(parent)
    , d_ptr(new InputRegionPrivate(this))
{
}

qreal InputRegion::x() const
{
    Q_D(const InputRegion);
    return d->x;
}

void InputRegion::setX(qreal x)
{
    Q_D(InputRegion);
    if (d->x != x) {
        d->x = x;
        d->update();
        emit xChanged();
    }
}

qreal InputRegion::y() const
{
    Q_D(const InputRegion);
    return d->y;
}

void InputRegion::setY(qreal y)
{
    Q_D(InputRegion);
    if (d->y != y) {
        d->y = y;
        d->update();
        emit yChanged();
    }
}

qreal InputRegion::width() const
{
    Q_D(const InputRegion);
    return d->width;
}

void InputRegion::setWidth(qreal width)
{
    Q_D(InputRegion);
    if (d->width != width) {
        d->width = width;
        d->update();
        emit widthChanged();
    }
}

qreal InputRegion::height() const
{
    Q_D(const InputRegion);
    return d->height;
}

void InputRegion::setHeight(qreal height)
{
    Q_D(InputRegion);
    if (d->height != height) {
        d->height = height;
        d->update();
        emit heightChanged();
    }
}

QWindow *InputRegion::window() const
{
    Q_D(const InputRegion);
    return d->window;
}

void InputRegion::setWindow(QWindow *window)
{
    Q_D(InputRegion);
    if (d->window != window) {
        d->window = window;
        d->update();
        emit windowChanged();
    }
}
