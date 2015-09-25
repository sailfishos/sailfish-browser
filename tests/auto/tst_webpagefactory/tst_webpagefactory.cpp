/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>

#include "webpagefactory.h"
#include "declarativewebpage.h"
#include "declarativewebcontainer.h"
#include "tab.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "WebPage {\n" \
        "}\n";

static const QByteArray QML_WRONG_SNIPPET = \
        "import QtQuick 2.0\n" \
        "Item {\n" \
        "}\n";

static const QByteArray QML_BROKEN_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "WebPage \n" \
        "}\n";

using ::testing::NiceMock;

class tst_webpagefactory : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void createWebPage_data();
    void createWebPage();
    void createWebPageUninitialized();

private:
    WebPageFactory *m_pageFactory;
};

void tst_webpagefactory::initTestCase()
{
    qmlRegisterType<NiceMock<DeclarativeWebPage> >("Sailfish.Browser", 1, 0, "WebPage");
}

void tst_webpagefactory::init()
{
    m_pageFactory = new WebPageFactory();
}

void tst_webpagefactory::cleanup()
{
    delete m_pageFactory;
}

void tst_webpagefactory::createWebPage_data()
{
    QTest::addColumn<QByteArray>("qml");
    QTest::addColumn<bool>("isSuccessExpected");

    QTest::newRow("ok_qml") << QML_SNIPPET << true;
    QTest::newRow("wrong_qml") << QML_WRONG_SNIPPET << false;
    QTest::newRow("broken_qml") << QML_BROKEN_SNIPPET << false;
}

void tst_webpagefactory::createWebPage()
{
    QFETCH(QByteArray, qml);
    QFETCH(bool, isSuccessExpected);

    DeclarativeWebPage* page;
    DeclarativeWebContainer webContainer;
    QQuickView view;
    QQmlComponent fakeComponent(view.engine());
    QQmlEngine::setContextForObject(&webContainer, view.engine()->rootContext());

    fakeComponent.setData(qml, QUrl());
    m_pageFactory->updateQmlComponent(&fakeComponent);
    if (isSuccessExpected) {
        EXPECT_CALL(webContainer, privateMode());
    }
    page = m_pageFactory->createWebPage(&webContainer, Tab(1, "http://example.com", "Title", ""), 0);
    QCOMPARE(!!page, isSuccessExpected);
}

void tst_webpagefactory::createWebPageUninitialized()
{
    DeclarativeWebContainer webContainer;
    QVERIFY(!m_pageFactory->createWebPage(&webContainer, Tab(1, "http://example.com", "Title", ""), 0));
}

QTEST_MAIN(tst_webpagefactory)
#include "tst_webpagefactory.moc"
