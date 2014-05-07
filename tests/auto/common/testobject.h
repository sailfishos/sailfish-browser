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
        "Item { width: 100; height: 100 }\n";

struct TestTab {
    TestTab(QString url, QString title) : url(url), title(title) {}

    QString url;
    QString title;
};

class TestObject : public QObject
{
    Q_OBJECT

public:
    explicit TestObject();
    explicit TestObject(QByteArray qmlData);

    void init(const QUrl &url);
    void waitSignals(QSignalSpy &spy, int expectedSignalCount) const;
    void setTestData(QByteArray qmlData);
    void setTestUrl(const QUrl &url);
    void setContextProperty(const QString &name, QObject *value);

    template <typename T> T *qmlObject(const char *propertyName) {
        QVariant var = mRootObject->property(propertyName);
        return qobject_cast<T *>(qvariant_cast<QObject*>(var));
    }

private:
    QObject *mRootObject;
    QQuickView mView;
};
