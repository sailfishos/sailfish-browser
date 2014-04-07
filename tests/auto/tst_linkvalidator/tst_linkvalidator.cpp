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
#include <QUrl>
#include "linkvalidator.h"

class tst_linkvalidator : public QObject
{
    Q_OBJECT

public:
    tst_linkvalidator(QObject *parent = 0);

private slots:
    void navigableUrls_data();
    void navigableUrls();

    void unnavigableUrls_data();
    void unnavigableUrls();
};


tst_linkvalidator::tst_linkvalidator(QObject *parent)
    : QObject(parent)
{
}

void tst_linkvalidator::navigableUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("http") << "http://foobar";
    QTest::newRow("https") << "https://foobar";
    QTest::newRow("file") << "file://foo/bar";
    QTest::newRow("relative link") << "foo/bar.html";
}

void tst_linkvalidator::navigableUrls()
{
    QFETCH(QString, url);
    QVERIFY(LinkValidator::navigable(QUrl(url)));
}

void tst_linkvalidator::unnavigableUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("sms") << "sms:+123456798";
    QTest::newRow("sms://") << "sms://+123456798";
    QTest::newRow("tel") << "tel:+123456798";
    QTest::newRow("tel://") << "tel://+123456798";
    QTest::newRow("mailto") << "mailto:joe@example.com";
    QTest::newRow("mailto://") << "mailto://joe@example.com";
    QTest::newRow("geo") << "geo:61.49464,23.77513";
    QTest::newRow("geo://") << "geo://61.49464,23.77513";
}

void tst_linkvalidator::unnavigableUrls()
{
    QFETCH(QString, url);
    QVERIFY(!LinkValidator::navigable(QUrl(url)));
}

QTEST_APPLESS_MAIN(tst_linkvalidator)

#include "tst_linkvalidator.moc"
