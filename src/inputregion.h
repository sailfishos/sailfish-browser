/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef INPUTREGION_H
#define INPUTREGION_H

#include <QObject>
#include <qqml.h>

class QWindow;
class InputRegionPrivate;

class InputRegion : public QObject
{
    Q_OBJECT
// add x, y
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged FINAL)
public:
    InputRegion(QObject *parent = 0);

    qreal x() const;
    void setX(qreal x);

    qreal y() const;
    void setY(qreal y);

    qreal width() const;
    void setWidth(qreal width);

    qreal height() const;
    void setHeight(qreal height);

    QWindow *window() const;
    void setWindow(QWindow *window);

signals:
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void windowChanged();

private:
    InputRegionPrivate *d_ptr;
    Q_DISABLE_COPY(InputRegion)
    Q_DECLARE_PRIVATE(InputRegion)
};

QML_DECLARE_TYPE(InputRegion)

#endif
