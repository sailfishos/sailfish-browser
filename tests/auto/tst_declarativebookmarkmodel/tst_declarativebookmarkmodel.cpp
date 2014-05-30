/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QObject>
#include <QtTest>
#include <QtQml>
#include <QFile>
#include <QDesktopServices>
#include <QGuiApplication>
#include "declarativebookmarkmodel.h"
#include "testobject.h"


static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   width: 100; height: 100\n" \
        "   property alias bookmarkModel: model\n" \
        "   BookmarkModel { id: model }\n" \
        "}\n";


class tst_declarativebookmarkmodel : public TestObject
{
    Q_OBJECT

public:
    tst_declarativebookmarkmodel();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void addBookmark();
    void removeBookmark();
    void contains();
    void editBookmark();

private:
    DeclarativeBookmarkModel *bookmarkModel;
};


tst_declarativebookmarkmodel::tst_declarativebookmarkmodel()
    : TestObject(QML_SNIPPET)
{
    bookmarkModel = TestObject::qmlObject<DeclarativeBookmarkModel>("bookmarkModel");
}

void tst_declarativebookmarkmodel::initTestCase()
{
    QVERIFY(bookmarkModel);

    // Preload bookmarks
    QCOMPARE(bookmarkModel->rowCount(), 5);
}

void tst_declarativebookmarkmodel::addBookmark()
{
    QSignalSpy countChangeSpy(bookmarkModel, SIGNAL(countChanged()));
    int count = bookmarkModel->rowCount() + 1;
    bookmarkModel->addBookmark("http://www.test1.jolla.com", "jolla", "");

    waitSignals(countChangeSpy, 1);
    QCOMPARE(bookmarkModel->rowCount(), count);
}

void tst_declarativebookmarkmodel::removeBookmark()
{
    bookmarkModel->addBookmark("http://www.test2.jolla.com", "jolla", "");
    int count = bookmarkModel->rowCount();

    QSignalSpy countChangeSpy(bookmarkModel, SIGNAL(countChanged()));
    bookmarkModel->removeBookmark("http://www.test2.jolla.com");
    waitSignals(countChangeSpy, 1);

    QCOMPARE(bookmarkModel->rowCount(), count-1);

    bookmarkModel->removeBookmark("http://www.test3.that.is.not.there.jolla.com");
    QCOMPARE(bookmarkModel->rowCount(), count-1);
}

void tst_declarativebookmarkmodel::contains()
{
    bookmarkModel->addBookmark("http://www.test4.contains.jolla.com", "jolla", "");

    QVERIFY(bookmarkModel->contains("http://www.test4.contains.jolla.com"));
    QVERIFY(!bookmarkModel->contains("http://www.test4.that.is.not.there.jolla.com"));
}

void tst_declarativebookmarkmodel::editBookmark()
{
    bookmarkModel->addBookmark("http://www.test5.jolla.com", "jolla", "");
    bookmarkModel->addBookmark("http://www.test5.jolla.com/2", "jolla2", "");
    int count = bookmarkModel->rowCount();

    QModelIndex index = bookmarkModel->index(0);
    // Let's make sure that first bookmark stays normal.
    QString constUrl = bookmarkModel->data(index, DeclarativeBookmarkModel::UrlRole).toString();
    QString constTitle = bookmarkModel->data(index, DeclarativeBookmarkModel::TitleRole).toString();

    // Edit only the last entry
    index = bookmarkModel->index(count-1);
    QString originalUrl = bookmarkModel->data(index, DeclarativeBookmarkModel::UrlRole).toString();
    QString newTitle("New title");
    QString newUrl("http://www.test5.jolla.com/edited");

    bookmarkModel->editBookmark(index.row(), originalUrl, newTitle);
    QString title = bookmarkModel->data(index, DeclarativeBookmarkModel::TitleRole).toString();
    QString url = bookmarkModel->data(index, DeclarativeBookmarkModel::UrlRole).toString();

    QCOMPARE(title, newTitle);
    QCOMPARE(url, originalUrl);

    bookmarkModel->editBookmark(index.row(), newUrl, title);
    title = bookmarkModel->data(index, DeclarativeBookmarkModel::TitleRole).toString();
    url = bookmarkModel->data(index, DeclarativeBookmarkModel::UrlRole).toString();

    QCOMPARE(title, newTitle);
    QCOMPARE(url, newUrl);

    index = bookmarkModel->index(0);
    url = bookmarkModel->data(index, DeclarativeBookmarkModel::UrlRole).toString();
    title = bookmarkModel->data(index, DeclarativeBookmarkModel::TitleRole).toString();

    QCOMPARE(title, constTitle);
    QCOMPARE(url, constUrl);

    QCOMPARE(count, bookmarkModel->rowCount());
}

void tst_declarativebookmarkmodel::cleanupTestCase()
{
    QFile file(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/bookmarks.json");
    QVERIFY(file.remove());
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("bookmarks-tests");
    app.setOrganizationName("org.sailfishos");
    app.setAttribute(Qt::AA_Use96Dpi, true);
    qmlRegisterType<DeclarativeBookmarkModel>("Sailfish.Browser", 1, 0, "BookmarkModel");
    tst_declarativebookmarkmodel testcase;
    return QTest::qExec(&testcase, argc, argv); \
}

#include "tst_declarativebookmarkmodel.moc"
