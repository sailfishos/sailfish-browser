/****************************************************************************
**
** Copyright (c) 2015 Jolla Ltd.
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

class tst_webutils : public QObject
{
    Q_OBJECT

public:
    tst_webutils(QObject *parent = 0);

private slots:
    void displayableUrl_data();
    void displayableUrl();
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

QTEST_MAIN(tst_webutils)

#include "tst_webutils.moc"
