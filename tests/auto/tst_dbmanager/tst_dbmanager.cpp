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
#include "dbmanager.h"
#include "testobject.h"

class tst_dbmanager : public TestObject
{
    Q_OBJECT

public:
    tst_dbmanager();


public slots:
    void tabsAvailableChanged(QList<Tab> tabs);

private slots:
    void addTabs_data();
    void addTabs();
    void getTabs();
    void updateThumbnailNonBlocking();
    void updateThumbnailBlocking();

    void cleanupTestCase();

private:
    void createTab(QString url, QString title);

    QList<Tab> currentTabs;
};


tst_dbmanager::tst_dbmanager()
    : TestObject(EMPTY_QML)
{
    connect(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)), this, SLOT(tabsAvailableChanged(QList<Tab>)));
}

void tst_dbmanager::tabsAvailableChanged(QList<Tab> tabs)
{
    currentTabs = tabs;
}

void tst_dbmanager::addTabs_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::newRow("http") << "http://foobar" << "FooBar 1";
    QTest::newRow("https") << "https://foobar" << "FooBar 2";
    QTest::newRow("file") << "file://foo/bar" << "FooBar 3";
}

void tst_dbmanager::addTabs()
{
    QFETCH(QString, url);
    QFETCH(QString, title);

    QBENCHMARK {
        createTab(url, title);
    }
}

void tst_dbmanager::getTabs()
{
    QSignalSpy tabsAvailableSpy(DBManager::instance(), SIGNAL(tabsAvailable(QList<Tab>)));
    QBENCHMARK {
        DBManager::instance()->getAllTabs();
    }
    waitSignals(tabsAvailableSpy, 1);
}

void tst_dbmanager::updateThumbnailNonBlocking()
{
    QVERIFY(currentTabs.count() > 0);
    int tabId = currentTabs.at(0).tabId();
    int counter = 0;

    QBENCHMARK {
        QString thumbnailPath = QString("foobar-%1.png").arg(counter);
        DBManager::instance()->updateThumbPath(tabId, thumbnailPath);
        ++counter;
    }
}

void tst_dbmanager::updateThumbnailBlocking()
{
    QVERIFY(currentTabs.count() > 0);
    int tabId = currentTabs.at(0).tabId();
    int counter = 0;

    QBENCHMARK {
        QSignalSpy thumbPathChangedSpy(DBManager::instance(), SIGNAL(thumbPathChanged(int,QString)));
        QString thumbnailPath = QString("foobar-%1.png").arg(counter);
        DBManager::instance()->updateThumbPath(tabId, thumbnailPath);
        waitSignals(thumbPathChangedSpy, 1);
        ++counter;
    }
}

void tst_dbmanager::cleanupTestCase()
{
    // Wait for event loop of db manager
    QTest::qWait(1000);
    QString dbFileName = QString("%1/%2")
            .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(dbFileName);
    QVERIFY(dbFile.remove());
}

void tst_dbmanager::createTab(QString url, QString title)
{
    DBManager::instance()->createTab(1000, url, title);
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_dbmanager testcase;
    return QTest::qExec(&testcase, argc, argv); \
}

#include "tst_dbmanager.moc"
