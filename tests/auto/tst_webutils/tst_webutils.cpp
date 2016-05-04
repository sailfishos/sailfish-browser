/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include "declarativewebutils.h"
#include "browserpaths.h"

static const QByteArray TEST_CONTENT = "Hello World!";

class tst_webutils : public QObject
{
    Q_OBJECT

public:
    tst_webutils(QObject *parent = 0);

private slots:
    void displayableUrl_data();
    void displayableUrl();
    void uniquePictureName_data();
    void uniquePictureName();
};

tst_webutils::tst_webutils(QObject *parent)
    : QObject(parent)
{
}

void tst_webutils::displayableUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expectedUrl");

    QTest::newRow("www1") << "http://www.test.com" << "test.com";
    QTest::newRow("www2") << "http://www.test.com/test1" << "test.com";

    QTest::newRow("m1") << "http://m.test.com" << "test.com";
    QTest::newRow("m2") << "http://m.test.com/test1" << "test.com";

    QTest::newRow("mobile1") << "http://mobile.test.com" << "test.com";
    QTest::newRow("mobile2") << "http://mobile.test.com/test1" << "test.com";

    QTest::newRow("app1") << "http://app.test.com" << "app.test.com";
    QTest::newRow("app2") << "http://app.test.com/test1" << "app.test.com";

    QTest::newRow("file") << "file:///home/test/" << "file:///home/test/";
}

void tst_webutils::displayableUrl()
{
    QFETCH(QString, url);
    QFETCH(QString, expectedUrl);

    DeclarativeWebUtils *webUtils = DeclarativeWebUtils::instance();
    QString resultUrl = webUtils->displayableUrl(url);
    QCOMPARE(resultUrl, expectedUrl);
}

void tst_webutils::uniquePictureName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QList<QString> >("existingFiles");
    QTest::addColumn<QString>("expectedName");

    QList<QString> existingFiles;
    QTest::newRow("new_file") << "some_picture.jpg" << existingFiles << "some_picture.jpg";

    QTest::newRow("new_file_no_ext") << "some_file" << existingFiles << "some_file";

    existingFiles << "some_picture.jpg";
    QTest::newRow("file_exists") << "some_picture.jpg" << existingFiles << "some_picture(2).jpg";

    QTest::newRow("new_file_2") << "some_picture.png" << existingFiles << "some_picture.png";

    existingFiles << "some_picture(2).jpg" << "some_picture(3).jpg";
    QTest::newRow("many_files_exist") << "some_picture.jpg" << existingFiles << "some_picture(4).jpg";

    existingFiles.clear();
    existingFiles << "some_picture.jpg" << "some_picture(3).jpg";
    QTest::newRow("many_files_exist_2") << "some_picture.jpg" << existingFiles << "some_picture(2).jpg";

    existingFiles.clear();
    existingFiles << "some_picture(2).jpg" << "some_picture(3).jpg";
    QTest::newRow("many_files_exist_3") << "some_picture.jpg" << existingFiles << "some_picture.jpg";

    existingFiles.clear();
    existingFiles << "and(2)_some(2).picture.jpg" << "and(2)_some(2)(2).picture.jpg";
    QTest::newRow("complicated_case") << "and(2)_some(2).picture.jpg" << existingFiles << "and(2)_some(2)(3).picture.jpg";

    existingFiles.clear();
    existingFiles << "some_file";
    QTest::newRow("file_exists_no_ext") << "some_file" << existingFiles << "some_file(2)";

    existingFiles << "some_file(2)" << "some_file(3)";
    QTest::newRow("many_files_exist_no_ext") << "some_file" << existingFiles << "some_file(4)";
}

void tst_webutils::uniquePictureName()
{
    QFETCH(QString, fileName);
    QFETCH(QList<QString>, existingFiles);
    QFETCH(QString, expectedName);

    // set up test case
    QDir dir(BrowserPaths::dataLocation());
    dir.removeRecursively();

    foreach (const QString& existingFile, existingFiles) {
        QFile file(BrowserPaths::dataLocation() + "/" + existingFile);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << TEST_CONTENT;
        file.close();
    }

    QString targetPath = BrowserPaths::dataLocation();

    // actual test
    DeclarativeWebUtils *webUtils = DeclarativeWebUtils::instance();
    QCOMPARE(webUtils->createUniqueFileUrl(fileName, targetPath), "file://" + targetPath + "/" + expectedName);

    // tear down
    // set up test case
    dir.removeRecursively();
    Q_UNUSED(BrowserPaths::dataLocation());
}

QTEST_MAIN(tst_webutils)

#include "tst_webutils.moc"
