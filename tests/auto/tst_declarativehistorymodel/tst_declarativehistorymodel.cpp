/****************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QCoreApplication>
#include <QtTest>
#include <QSignalSpy>
#include <qqml.h>

#include "declarativehistorymodel.h"
#include "testobject.h"
#include "dbmanager.h"
#include "browserpaths.h"

struct HistoryEntry {
    HistoryEntry(QString url, QString title) : url(url), title(title) {}
    HistoryEntry() {}

    QString url;
    QString title;
};

Q_DECLARE_METATYPE(HistoryEntry)

class tst_declarativehistorymodel : public TestObject
{
    Q_OBJECT

public:
    tst_declarativehistorymodel();

private slots:
    void initTestCase();
    void init();

    void addNonSameHistoryEntries_data();
    void addNonSameHistoryEntries();
    void addDuplicateHistoryEntries_data();
    void addDuplicateHistoryEntries();

    void sortedHistoryEntries_data();
    void sortedHistoryEntries();

    void emptyTitles_data();
    void emptyTitles();

    void removeHistoryEntries_data();
    void removeHistoryEntries();

    void searchWithSpecialChars_data();
    void searchWithSpecialChars();

    void cleanup();

private:
    void addEntries(const QList<HistoryEntry> &entries);
    void verifySearchResult(QString searchTerm, int expectedCount);

    DeclarativeHistoryModel *historyModel;
    QString dbFileName;
};


tst_declarativehistorymodel::tst_declarativehistorymodel()
    : TestObject()
{
}

void tst_declarativehistorymodel::initTestCase()
{

    dbFileName = QString("%1/%2")
            .arg(BrowserPaths::dataLocation())
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(dbFileName);
    dbFile.remove();
}

void tst_declarativehistorymodel::init()
{
    historyModel = new DeclarativeHistoryModel;

    QVERIFY(historyModel);

    historyModel->componentComplete();
    QSignalSpy populatedSpy(historyModel, SIGNAL(populated()));
    // History must be loaded with in 500ms
    QVERIFY(populatedSpy.wait());
    QCOMPARE(populatedSpy.count(), 1);

}

void tst_declarativehistorymodel::addNonSameHistoryEntries_data()
{
    QTest::addColumn<QList<HistoryEntry> >("entries");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");

    QList<HistoryEntry> list {
        HistoryEntry(QStringLiteral("http://www.foobar.com/page1/"), QStringLiteral("FooBar Page1")),
        HistoryEntry(QStringLiteral("http://www.foobar.com/page2/"), QStringLiteral("FooBar Page2")),
        HistoryEntry(QStringLiteral("http://www.foobar.com/page3/"), QStringLiteral("FooBar Page3"))
    };

    QTest::newRow("foo1") << list << "Page1" << 1;
    QTest::newRow("foo2") << list << "Page2" << 1;
    QTest::newRow("foo3") << list << "Page3" << 1;
}

void tst_declarativehistorymodel::addNonSameHistoryEntries()
{
    QFETCH(QList<HistoryEntry>, entries);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    addEntries(entries);
    verifySearchResult("", entries.count());
    verifySearchResult(searchTerm, expectedCount);
}

void tst_declarativehistorymodel::addDuplicateHistoryEntries_data()
{
    QTest::addColumn<QList<HistoryEntry> >("entries");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");

    QList<HistoryEntry> list {
        HistoryEntry(QStringLiteral("http://www.foobar.com/page1/"), QStringLiteral("FooBar Page1")),
        HistoryEntry(QStringLiteral("http://www.foobar.com/page1/"), QStringLiteral("FooBar Page1")),
        HistoryEntry(QStringLiteral("http://www.foobar.com/page2/"), QStringLiteral("FooBar Page2")),
        HistoryEntry(QStringLiteral("http://www.foobar.com/page2/"), QStringLiteral("FooBar Page2")),
        HistoryEntry(QStringLiteral("http://www.foobar.com/page3/"), QStringLiteral("FooBar Page3")),
        HistoryEntry(QStringLiteral("http://www.foobar.com/page3/"), QStringLiteral("FooBar Page3"))
    };

    QTest::newRow("foo1") << list << "Page1" << 1;
    QTest::newRow("foo2") << list << "Page2" << 1;
    QTest::newRow("foo3") << list << "Page3" << 1;
}

void tst_declarativehistorymodel::addDuplicateHistoryEntries()
{
    QFETCH(QList<HistoryEntry>, entries);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    addEntries(entries);
    verifySearchResult("", 3);
    verifySearchResult(searchTerm, expectedCount);
}

void tst_declarativehistorymodel::sortedHistoryEntries_data()
{
    QTest::addColumn<QList<HistoryEntry> >("entries");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<QStringList>("order");
    QTest::addColumn<int>("expectedCount");


    QList<HistoryEntry> list {
        HistoryEntry(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url")),
    };


    // Insert in reversed order
    QTest::newRow("longestUrl") << (QList<HistoryEntry>() <<
                                   HistoryEntry(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url"))) << "test"
                              << (QStringList() << "http://www.testurl.blah/thelongesturl/") << 1;
    QTest::newRow("longerUrl") << (QList<HistoryEntry>() <<
                                   HistoryEntry(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url")) <<
                                   HistoryEntry(QStringLiteral("http://www.testurl.blah/alongerurl/"), QStringLiteral("A longer url")))
                               << "test" << (QStringList() << "http://www.testurl.blah/alongerurl/"
                                   << "http://www.testurl.blah/thelongesturl/") << 2;

    QTest::newRow("rootPage") << (QList<HistoryEntry>() <<
                                  HistoryEntry(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url")) <<
                                  HistoryEntry(QStringLiteral("http://www.testurl.blah/alongerurl/"), QStringLiteral("A longer url")) <<
                                  HistoryEntry(QStringLiteral("http://www.testurl.blah/"), QStringLiteral("A root page")))
                              << "test"
                              << (QStringList() << "http://www.testurl.blah/" << "http://www.testurl.blah/alongerurl/"
                                  << "http://www.testurl.blah/thelongesturl/") << 3;
}

void tst_declarativehistorymodel::sortedHistoryEntries()
{
    QFETCH(QList<HistoryEntry>, entries);
    QFETCH(QString, searchTerm);
    QFETCH(QStringList, order);
    QFETCH(int, expectedCount);

    addEntries(entries);
    verifySearchResult(searchTerm, expectedCount);

    for (int i = 0; i < expectedCount; ++i) {
        QModelIndex modelIndex = historyModel->createIndex(i, 0);
        QString url = historyModel->data(modelIndex, DeclarativeHistoryModel::UrlRole).toString();
        QCOMPARE(url, order.at(i));
    }
}

void tst_declarativehistorymodel::emptyTitles_data()
{
    QTest::addColumn<QList<HistoryEntry> >("entries");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<QString>("expectedTitle");

    auto longUrl = QStringLiteral("http://www.testurl.blah/thelongesturl/");
    auto longTitle = QStringLiteral("The longest url");
    auto quickUrl = QStringLiteral("http://quick");

    QTest::newRow("duplicate_longestUrl") << (QList<HistoryEntry>()
                                              << HistoryEntry(longUrl, QString())
                                              << HistoryEntry(longUrl, longTitle))
                                          << "test" << longTitle;
    QTest::newRow("random_url") << (QList<HistoryEntry>() << HistoryEntry(quickUrl, QString()))
                                << "quick" << quickUrl;
}

void tst_declarativehistorymodel::emptyTitles()
{
    QFETCH(QList<HistoryEntry>, entries);
    QFETCH(QString, searchTerm);
    QFETCH(QString, expectedTitle);

    addEntries(entries);
    verifySearchResult(searchTerm, 1);

    QModelIndex modelIndex = historyModel->createIndex(0, 0);
    QString title = historyModel->data(modelIndex, DeclarativeHistoryModel::TitleRole).toString();

    QCOMPARE(title, expectedTitle);
}

void tst_declarativehistorymodel::removeHistoryEntries_data()
{
    QStringList urls;
    urls << "http://removeTestUrl1" << "http://removeTestUrl2" << "http://removeTestUrl3";
    QStringList titles;
    titles << "test1" << "test2" << "test3";


    QList<HistoryEntry> list {
        HistoryEntry(QStringLiteral("http://removeTestUrl1"), QStringLiteral("test1")),
        HistoryEntry(QStringLiteral("http://removeTestUrl2"), QStringLiteral("test2")),
        HistoryEntry(QStringLiteral("http://removeTestUrl3"), QStringLiteral("test3")),
    };

    QTest::addColumn<QList<HistoryEntry> >("entries");
    QTest::addColumn<int>("index");
    QTest::addColumn<int>("countWithSearchTermIndexRemoved");
    QTest::addColumn<int>("countWithEmptySearchIndexRemoved");
    QTest::addColumn<int>("countWithSearchTerm");
    QTest::addColumn<QString>("searchTerm");
    QTest::newRow("remove_first") << list << 0 << 0 << 2 << 1 << list.at(0).url;
    QTest::newRow("remove_middle") << list << 1 << 0 << 2 << 1  << list.at(1).url;
    QTest::newRow("remove_last") << list << 2 << 0 << 2 << 1 << list.at(2).url;
    QTest::newRow("out_of_bounds_negative") << list << -1 << 3 << 3 << 3 << "removeTestUrl";
    QTest::newRow("out_of_bounds_positive") << list << 4 << 3 << 3 << 3 << "removeTestUrl";
}

void tst_declarativehistorymodel::removeHistoryEntries()
{
    QFETCH(QList<HistoryEntry>, entries);
    QFETCH(int, index);
    QFETCH(int, countWithSearchTermIndexRemoved);
    QFETCH(int, countWithEmptySearchIndexRemoved);
    QFETCH(int, countWithSearchTerm);

    QFETCH(QString, searchTerm);
    addEntries(entries);
    verifySearchResult(searchTerm, countWithSearchTerm);
    // Reset search results.
    verifySearchResult("", entries.count());

    if (index >= 0 && index < entries.count()) {
        // Remove by url rather than index, since the ordering may change
        historyModel->remove(entries[index].url);
    } else {
        // Out of bounds test
        historyModel->remove(index);
    }
    verifySearchResult("", countWithEmptySearchIndexRemoved);

    historyModel->search(searchTerm);
    verifySearchResult(searchTerm, countWithSearchTermIndexRemoved);
}

void tst_declarativehistorymodel::searchWithSpecialChars_data()
{
    QTest::addColumn<QList<HistoryEntry> >("entries");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");
    QTest::newRow("special_site") << (QList<HistoryEntry>() << HistoryEntry(QStringLiteral("http://www.pöö.com/"), QStringLiteral("wierd site"))) << "pöö" << 1;
    QTest::newRow("special_title") << (QList<HistoryEntry>() << HistoryEntry(QStringLiteral("http://www.pöö.com/"), QStringLiteral("wierd site"))
                                      << HistoryEntry(QStringLiteral("http://www.foobar.com/"), QStringLiteral("pöö wierd title"))) << "pöö" << 2;

    QTest::newRow("special_escaped_chars") << (QList<HistoryEntry>() << HistoryEntry(QStringLiteral("http://www.foobar.com/"), QStringLiteral("special title: ';\";ö"))) << "';\";" << 1;
    QTest::newRow("special_escaped_chars") << (QList<HistoryEntry>() << HistoryEntry(QStringLiteral("http://www.foobar.com/"), QStringLiteral("Ö is wierd char"))) << "Ö" << 1;
}

void tst_declarativehistorymodel::searchWithSpecialChars()
{
    QFETCH(QList<HistoryEntry>, entries);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    addEntries(entries);

    verifySearchResult(searchTerm, expectedCount);

    // Wierdly this works in unit test, but in production code doesn't, perhaps linking to different sqlite version
    // QEXPECT_FAIL("special_upper_case_special_char", "due to sqlite bug accented char is case sensitive with LIKE op", Continue);
}

void tst_declarativehistorymodel::cleanup()
{
    delete historyModel;
    historyModel = 0;
    delete DBManager::instance();
    QFile dbFile(dbFileName);
    QVERIFY(dbFile.remove());
}

void tst_declarativehistorymodel::addEntries(const QList<HistoryEntry> &entiries)
{
    QSignalSpy historyAvailable(DBManager::instance(), SIGNAL(historyAvailable(QList<Link>)));
    for (int i = 0; i < entiries.count(); ++i) {
        historyModel->add(entiries.at(i).url, entiries.at(i).title);
    }

    waitSignals(historyAvailable, entiries.count());
}

void tst_declarativehistorymodel::verifySearchResult(QString searchTerm, int expectedCount)
{
    QSignalSpy historyAvailable(DBManager::instance(), SIGNAL(historyAvailable(QList<Link>)));
    historyModel->search(searchTerm);
    waitSignals(historyAvailable, 1, 500);
    QCOMPARE(historyModel->rowCount(), expectedCount);
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_declarativehistorymodel testcase;
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativehistorymodel.moc"
