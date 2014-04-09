/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QGuiApplication>
#include <QtTest>
#include <QSignalSpy>
#include <qqml.h>

#include "declarativetabmodel.h"
#include "declarativehistorymodel.h"
#include "testobject.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   width: 100; height: 100\n" \
        "   property alias tabModel: tabModel\n" \
        "   property alias historyModel: historyModel\n" \
        "   TabModel { id: tabModel }\n" \
        "   HistoryModel { id: historyModel }\n" \
        "}\n";


class tst_declarativehistorymodel : public TestObject
{
    Q_OBJECT

public:
    tst_declarativehistorymodel();

private slots:
    void initTestCase();

    void addNonSameHistoryEntries_data();
    void addNonSameHistoryEntries();
    void addDuplicateHistoryEntries_data();
    void addDuplicateHistoryEntries();

    void cleanupTestCase();

private:
    DeclarativeHistoryModel *historyModel;
    DeclarativeTabModel *tabModel;
};


tst_declarativehistorymodel::tst_declarativehistorymodel()
    : TestObject(QML_SNIPPET)
{
    tabModel = TestObject::model<DeclarativeTabModel>("tabModel");
    historyModel = TestObject::model<DeclarativeHistoryModel>("historyModel");
}

void tst_declarativehistorymodel::initTestCase()
{
    QVERIFY(tabModel);
    QVERIFY(historyModel);

    if (!tabModel->loaded()) {
        QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
        // Tabs must be loaded with in 500ms
        QVERIFY(loadedSpy.wait());
        QCOMPARE(loadedSpy.count(), 1);
    }
}

void tst_declarativehistorymodel::addNonSameHistoryEntries_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");
    QTest::newRow("foo1") << "http://www.foobar.com/page1/" << "FooBar Page1" << "Page1" << 1;
    QTest::newRow("foo2") << "http://www.foobar.com/page2/" << "FooBar Page2" << "Page2" << 2;
    QTest::newRow("foo3") << "http://www.foobar.com/page3/" << "FooBar Page3" << "Page3" << 3;
}

void tst_declarativehistorymodel::addNonSameHistoryEntries()
{
    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    tabModel->addTab(url, title);

    historyModel->search("");
    QTest::qWait(1000);
    QCOMPARE(historyModel->rowCount(), expectedCount);

    historyModel->search(searchTerm);
    QTest::qWait(1000);
    QCOMPARE(historyModel->rowCount(), 1);
}

void tst_declarativehistorymodel::addDuplicateHistoryEntries_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");
    QTest::newRow("foo1") << "http://www.foobar.com/page1/" << "FooBar Page1" << "Page1" << 3;
    QTest::newRow("foo2") << "http://www.foobar.com/page2/" << "FooBar Page2" << "Page2" << 3;
    QTest::newRow("foo3") << "http://www.foobar.com/page3/" << "FooBar Page3" << "Page3" << 3;
}

void tst_declarativehistorymodel::addDuplicateHistoryEntries()
{
    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    QSignalSpy countChangeSpy(historyModel, SIGNAL(countChanged()));

    tabModel->addTab(url, title);

    historyModel->search("");
    waitSignals(countChangeSpy, 1);
    QCOMPARE(historyModel->rowCount(), expectedCount);

    historyModel->search(searchTerm);
    waitSignals(countChangeSpy, 2);
    QCOMPARE(historyModel->rowCount(), 1);
}

void tst_declarativehistorymodel::cleanupTestCase()
{
    tabModel->clear();
    QVERIFY(tabModel->count() == 0);

    // Wait for event loop of db manager
    QTest::qWait(1000);
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
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    qmlRegisterType<DeclarativeHistoryModel>("Sailfish.Browser", 1, 0, "HistoryModel");
    tst_declarativehistorymodel testcase;
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativehistorymodel.moc"
