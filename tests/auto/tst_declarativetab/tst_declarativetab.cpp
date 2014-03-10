/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickView>
#include <QHash>
#include <QStandardPaths>

#include "declarativetab.h"
#include "declarativetabmodel.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   property alias tabModel: model\n" \
        "   property alias tabItem: tab\n" \
        "   width: 100; height: 100\n" \
        "   Tab { id: tab }\n" \
        "   TabModel { id: model;  currentTab: tab }\n" \
        "}\n";

class tst_declarativetab : public QObject
{
    Q_OBJECT

public:
    tst_declarativetab(QObject *parent = 0);

private slots:
    void initTestCase();
    void cleanupTestCase();

private:
    DeclarativeTabModel *tabModel;
    DeclarativeTab *tab;
    QQuickView view;
};


tst_declarativetab::tst_declarativetab(QObject *parent)
    : QObject(parent)
    , tabModel(0)
    , tab(0)
{
}

void tst_declarativetab::initTestCase()
{
    QQmlComponent component(view.engine());
    component.setData(QML_SNIPPET, QUrl());
    QObject *obj = component.create(view.engine()->rootContext());

    view.setContent(QUrl(""), 0, obj);
    view.show();
    QTest::qWaitForWindowExposed(&view);
    QVariant var = obj->property("tabModel");
    tabModel = qobject_cast<DeclarativeTabModel *>(qvariant_cast<QObject*>(var));
    QVERIFY(tabModel);

    var = obj->property("tabItem");
    tab = qobject_cast<DeclarativeTab *>(qvariant_cast<QObject*>(var));
    QVERIFY(tab);

    if (!tabModel->loaded()) {
        QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
        // Tabs must be loaded with in 500ms
        QVERIFY(loadedSpy.wait());
        QCOMPARE(loadedSpy.count(), 1);
    }

    QSignalSpy currentUrlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy forwardSpy(tab, SIGNAL(canGoFowardChanged()));
    QSignalSpy backSpy(tab, SIGNAL(canGoBackChanged()));
    tabModel->addTab("http://www.jolla.com", "");
    QCOMPARE(currentUrlChangedSpy.count(), 1);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(backSpy.count(), 0);
}

void tst_declarativetab::cleanupTestCase()
{
    tabModel->clear();
    QVERIFY(tabModel->count() == 0);

    // Wait for event loop of db manager
    QTest::qWait(500);
    QString dbFileName = QString("%1/%2")
            .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(dbFileName);
    QVERIFY(dbFile.remove());
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_declarativetab testcase;
    qmlRegisterType<DeclarativeTab>("Sailfish.Browser", 1, 0, "Tab");
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativetab.moc"
