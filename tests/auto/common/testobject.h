/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QObject>
#include <QByteArray>
#include <QQuickView>

class DeclarativeTabModel;
class DeclarativeHistoryModel;
class QSignalSpy;

static const QByteArray EMPTY_QML = \
        "import QtQuick 2.0\n" \
        "Item {}\n";

class TestObject : public QObject
{
    Q_OBJECT

public:
    explicit TestObject(QByteArray qmlData);

    void waitSignals(QSignalSpy &spy, int expectedSignalCount) const;
    void setTestData(QByteArray qmlData);

    template <typename T> T *model(const char *propertyName) {
        QVariant var = mRootObject->property(propertyName);
        return qobject_cast<T *>(qvariant_cast<QObject*>(var));
    }

private:
    QObject *mRootObject;
    QQuickView mView;
};
