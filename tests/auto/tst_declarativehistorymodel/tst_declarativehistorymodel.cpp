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

#include "persistenttabmodel.h"
#include "declarativehistorymodel.h"
#include "testobject.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   width: 100; height: 100\n" \
        "   property alias tabModel: tabModel\n" \
        "   property alias historyModel: historyModel\n" \
        "   PersistentTabModel { id: tabModel }\n" \
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

    void sortedHistoryEntries_data();
    void sortedHistoryEntries();

    void emptyTitles_data();
    void emptyTitles();

    void searchWithSpecialChars_data();
    void searchWithSpecialChars();

    void cleanupTestCase();

private:
    DeclarativeHistoryModel *historyModel;
    DeclarativeTabModel *tabModel;
};


tst_declarativehistorymodel::tst_declarativehistorymodel()
    : TestObject(QML_SNIPPET)
{
    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    historyModel = TestObject::qmlObject<DeclarativeHistoryModel>("historyModel");
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

void tst_declarativehistorymodel::sortedHistoryEntries_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<QStringList>("order");
    QTest::addColumn<int>("expectedCount");
    // Insert in reversed order
    QTest::newRow("longestUrl") << "http://www.testurl.blah/thelongesturl/"
                                << "The longest url" << "test"
                              << (QStringList() << "http://www.testurl.blah/thelongesturl/") << 1;
    QTest::newRow("longerUrl") << "http://www.testurl.blah/alongerurl/" << "A longer url" << "test"
                               << (QStringList() << "http://www.testurl.blah/alongerurl/"
                                   << "http://www.testurl.blah/thelongesturl/") << 2;
    QTest::newRow("rootPage") << "http://www.testurl.blah/" << "A root page" << "test"
                              << (QStringList() << "http://www.testurl.blah/" << "http://www.testurl.blah/alongerurl/"
                                  << "http://www.testurl.blah/thelongesturl/") << 3;
}

void tst_declarativehistorymodel::sortedHistoryEntries()
{
    // Clear previous search term / history count.
    QSignalSpy countChangeSpy(historyModel, SIGNAL(countChanged()));
    historyModel->search("");
    waitSignals(countChangeSpy, 1);

    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(QString, searchTerm);
    QFETCH(QStringList, order);
    QFETCH(int, expectedCount);

    tabModel->addTab(url, title);

    historyModel->search(searchTerm);
    waitSignals(countChangeSpy, 2);
    QCOMPARE(historyModel->rowCount(), expectedCount);

    for (int i = 0; i < expectedCount; ++i) {
        QModelIndex modelIndex = historyModel->createIndex(i, 0);
        QString url = historyModel->data(modelIndex, DeclarativeHistoryModel::UrlRole).toString();
        QCOMPARE(url, order.at(i));
    }
}

void tst_declarativehistorymodel::emptyTitles_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");

    QTest::newRow("duplicate_longestUrl") << "http://www.testurl.blah/thelongesturl/" << "test" << 3;
    QTest::newRow("random_url") << "http://quick/" << "quick" << 0;
}

void tst_declarativehistorymodel::emptyTitles()
{
    // Clear previous search term / history count.
    QSignalSpy countChangeSpy(historyModel, SIGNAL(countChanged()));
    historyModel->search("");
    waitSignals(countChangeSpy, 1);

    QFETCH(QString, url);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);
    tabModel->addTab(url, "");

    historyModel->search(searchTerm);
    waitSignals(countChangeSpy, 2);
    QCOMPARE(historyModel->rowCount(), expectedCount);

    for (int i = 0; i < expectedCount; ++i) {
        QModelIndex modelIndex = historyModel->createIndex(i, 0);
        QString title = historyModel->data(modelIndex, DeclarativeHistoryModel::TitleRole).toString();
        QVERIFY(!title.isEmpty());
    }
}


void tst_declarativehistorymodel::searchWithSpecialChars_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");
    QTest::newRow("special_site") << "http://www.pöö.com/" << "wierd site" << "pöö" << 1;
    QTest::newRow("special_title") << "http://www.foobar.com/" << "pöö wierd title" << "pöö" << 2;
    QTest::newRow("special_escaped_chars") << "http://www.foobar.com/" << "special title: ';\";ö" << "';\";" << 1;
    QTest::newRow("special_upper_case_special_char") << "http://www.foobar.com/" << "Ö is wierd char" << "Ö" << 2;
}

void tst_declarativehistorymodel::searchWithSpecialChars()
{
    // Clear previous search term / history count.
    QSignalSpy countChangeSpy(historyModel, SIGNAL(countChanged()));
    historyModel->search("");
    waitSignals(countChangeSpy, 1);

    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    tabModel->addTab(url, title);
    historyModel->search(searchTerm);
    waitSignals(countChangeSpy, 2);

    // Wierdly this works in unit test, but in production code doesn't, perhaps linking to different sqlite version
    // QEXPECT_FAIL("special_upper_case_special_char", "due to sqlite bug accented char is case sensitive with LIKE op", Continue);
    QCOMPARE(historyModel->rowCount(), expectedCount);
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
    qmlRegisterType<PersistentTabModel>("Sailfish.Browser", 1, 0, "PersistentTabModel");
    qmlRegisterType<DeclarativeHistoryModel>("Sailfish.Browser", 1, 0, "HistoryModel");
    tst_declarativehistorymodel testcase;
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativehistorymodel.moc"
