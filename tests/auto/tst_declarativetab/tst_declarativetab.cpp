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

    void tabUpdate();

    // Navigate forward and back (check url, title changes)
    void forwardBackwardNavigation();

    void navigateToInvalidUrls_data();
    void navigateToInvalidUrls();

    void updateInvalidUrls_data();
    void updateInvalidUrls();

    void updateValidUrls_data();
    void updateValidUrls();

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

void tst_declarativetab::tabUpdate()
{
    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));
    QSignalSpy forwardSpy(tab, SIGNAL(canGoFowardChanged()));
    QSignalSpy backSpy(tab, SIGNAL(canGoBackChanged()));

    tab->updateTab(tab->url(), "Jolla");

    QCOMPARE(urlChangedSpy.count(), 0);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(backSpy.count(), 0);

    tab->updateTab(tab->url(), "Jolla -- we are unlike!");
    QCOMPARE(urlChangedSpy.count(), 0);
    QCOMPARE(titleChangedSpy.count(), 2);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(backSpy.count(), 0);
}

void tst_declarativetab::forwardBackwardNavigation()
{
    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));
    QSignalSpy thumbChangedSpy(tab, SIGNAL(thumbPathChanged(QString,int)));
    QSignalSpy forwardSpy(tab, SIGNAL(canGoFowardChanged()));
    QSignalSpy backSpy(tab, SIGNAL(canGoBackChanged()));

    QString url("http://sailfishos.org");
    tab->navigateTo(url);

    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(forwardSpy.count(), 0);

    QCOMPARE(tab->url(), url);
    QVERIFY(tab->title().isEmpty());
    QVERIFY(tab->thumbnailPath().isEmpty());

    backSpy.wait();
    QCOMPARE(backSpy.count(), 1);
    QVERIFY(tab->canGoBack());

    QVERIFY(tab->valid());
    tab->captureScreen(url, 0, 0, 100, 100, 0);
    thumbChangedSpy.wait();
    QCOMPARE(thumbChangedSpy.count(), 1);
    QString path = QString("%1/tab-%2-thumb.png").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).arg(tab->tabId());
    QCOMPARE(tab->thumbnailPath(), path);

    QString title("SailfishOS.org");
    tab->updateTab(url, title);

    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 2);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(backSpy.count(), 1);
    QCOMPARE(tab->title(), title);

    tab->goBack();
    forwardSpy.wait();
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 3);

    QVERIFY(!tab->canGoBack());
    QVERIFY(tab->canGoForward());

    url = "http://www.jolla.com";
    title = "Jolla -- we are unlike!";
    QCOMPARE(tab->url(), url);
    QCOMPARE(tab->title(), title);

    // Verify that spy counters will not update (1sec should be enough)
    QTest::qWait(1000);
    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 3);

    tab->goForward();
    backSpy.wait();

    url = "http://sailfishos.org";
    title = "SailfishOS.org";
    QCOMPARE(tab->url(), url);
    QCOMPARE(tab->title(), title);

    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 3);
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 4);

    QVERIFY(tab->canGoBack());
    QVERIFY(!tab->canGoForward());

    forwardSpy.clear();
    backSpy.clear();
    urlChangedSpy.clear();
    titleChangedSpy.clear();
    tab->goBack();
    forwardSpy.wait();

    QCOMPARE(forwardSpy.count(), 1);
    QCOMPARE(backSpy.count(), 1);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);

    QVERIFY(!tab->canGoBack());
    QVERIFY(tab->canGoForward());

    url = "https://sailfishos.org/sailfish-silica/index.html";
    tab->navigateTo(url);
    forwardSpy.wait();
    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);

    QVERIFY(tab->title().isEmpty());
    QVERIFY(tab->thumbnailPath().isEmpty());
    QVERIFY(tab->canGoBack());
    QVERIFY(!tab->canGoForward());

    title = "Creating applications Sailfish Silica";
    tab->updateTab(url, title);
    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 3);
    QCOMPARE(tab->title(), title);

    url = "https://sailfishos.org/sailfish-silica/sailfish-silica-introduction.html";
    tab->navigateTo(url);
    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 4);
    QCOMPARE(tab->url(), url);
    QVERIFY(tab->title().isEmpty());
    QVERIFY(tab->canGoBack());
    QVERIFY(!tab->canGoForward());

    // Wait and check that all updates have come already
    QTest::qWait(1000);
    QCOMPARE(forwardSpy.count(), 2);
    QCOMPARE(backSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 3);
    QCOMPARE(titleChangedSpy.count(), 4);
}

void tst_declarativetab::navigateToInvalidUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("tel") << "tel:+123456798";
    QTest::newRow("sms") << "sms:+123456798";
    QTest::newRow("mailto") << "mailto:joe@example.com";
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1";
    QTest::newRow("geo") << "geo:61.49464,23.77513";
    QTest::newRow("geo://") << "geo://61.49464,23.77513";
}

void tst_declarativetab::navigateToInvalidUrls()
{
    tab->updateTab("http://foobar", "FooBar");

    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));

    QFETCH(QString, url);
    tab->navigateTo(url);

    QCOMPARE(urlChangedSpy.count(), 0);
    QCOMPARE(titleChangedSpy.count(), 0);
}

void tst_declarativetab::updateInvalidUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::newRow("tel") << "tel:+123456798" << "tel";
    QTest::newRow("sms") << "sms:+123456798" << "sms";;
    QTest::newRow("mailto") << "mailto:joe@example.com" << "1st mailto";
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1"  << "2nd mailto";
    QTest::newRow("geo") << "geo:61.49464,23.77513" << "1st geo";
    QTest::newRow("geo://") << "geo://61.49464,23.77513" << "2nd geo";
}

void tst_declarativetab::updateInvalidUrls()
{
    QString expectedUrl = "http://foobar/invalid";
    QString expectedTitle = "Invalid FooBar";
    tab->updateTab(expectedUrl, expectedTitle);

    QFETCH(QString, url);
    QFETCH(QString, title);

    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));

    tab->updateTab(url, title);

    QCOMPARE(urlChangedSpy.count(), 0);
    QCOMPARE(titleChangedSpy.count(), 0);
    QCOMPARE(tab->url(), expectedUrl);
    QCOMPARE(tab->title(), expectedTitle);
}

void tst_declarativetab::updateValidUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::newRow("http") << "http://foobar" << "FooBar";
    QTest::newRow("https") << "https://foobar" << "FooBar 2";
    QTest::newRow("file") << "file://foo/bar/index.html" << "Local foobar";
    QTest::newRow("relative") << "foo/bar/index.html" << "Relative foobar";
}

void tst_declarativetab::updateValidUrls()
{
    QFETCH(QString, url);
    QFETCH(QString, title);

    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));

    tab->updateTab(url, title);

    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);
    QCOMPARE(tab->url(), url);
    QCOMPARE(tab->title(), title);
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
