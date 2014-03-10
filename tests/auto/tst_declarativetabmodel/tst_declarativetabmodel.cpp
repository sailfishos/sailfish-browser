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

#include "declarativetab.h"
#include "declarativetabmodel.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   property alias tabModel: model\n" \
        "   width: 100; height: 100\n" \
        "   Tab { id: tab }\n" \
        "   TabModel { id: model;  currentTab: tab }\n" \
        "}\n";

class tst_declarativetabmodel : public QObject
{
    Q_OBJECT

public:
    tst_declarativetabmodel(QObject *parent = 0);

private slots:
    void initTestCase();
    void cleanupTestCase();

    void validTabs_data();
    void validTabs();

    void activateTabs();
    void tabUpdate();

    // Navigate forward and back (check url, title changes)
    void forwardBackwardNavigation();

    void navigateToInvalidUrls_data();
    void navigateToInvalidUrls();

    void updateInvalidUrls_data();
    void updateInvalidUrls();

    void updateValidUrls_data();
    void updateValidUrls();

    void remove();
    void closeActiveTab();

    void invalidTabs_data();
    void invalidTabs();
    void clear();

private:
    QStringList modelToStringList(const DeclarativeTabModel *tabModel) const;

    DeclarativeTabModel *tabModel;
    QQuickView view;
    const QStringList originalTabOrder;
};

tst_declarativetabmodel::tst_declarativetabmodel(QObject *parent)
    : QObject(parent)
    , tabModel(0)
    , originalTabOrder(QStringList() << "http://sailfishos.org"
                                     << "file:///opt/tests/sailfish-browser/manual/testpage.html"
                                     << "https://sailfishos.org/sailfish-silica/index.html"
                                     << "http://www.jolla.com")
{
}

void tst_declarativetabmodel::initTestCase()
{
    QQmlComponent component(view.engine());
    component.setData(QML_SNIPPET, QUrl());
    QObject *obj = component.create(view.engine()->rootContext());

    view.show();
    QTest::qWaitForWindowExposed(&view);
    QVariant var = obj->property("tabModel");
    tabModel = qobject_cast<DeclarativeTabModel *>(qvariant_cast<QObject*>(var));
    QVERIFY(tabModel);

    if (!tabModel->loaded()) {
        QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
        // Tabs must be loaded with in 500ms
        QVERIFY(loadedSpy.wait());
        QCOMPARE(loadedSpy.count(), 1);
    }
}

void tst_declarativetabmodel::cleanupTestCase()
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

void tst_declarativetabmodel::validTabs_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::addColumn<int>("count");

    for (int i = 0; i < originalTabOrder.count(); ++i) {
        const char *newName = QString("%1-tab").arg(i+1).toLatin1().constData();
        QTest::newRow(newName) << originalTabOrder.at(i) << QString("title-%1").arg(i+1) << i+1;
    }
}

void tst_declarativetabmodel::validTabs()
{
    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));
    QSignalSpy currentUrlChangedSpy(tabModel->currentTab(), SIGNAL(urlChanged()));

    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(int, count);

    QString previousActiveUrl = tabModel->currentTab()->url();

    tabModel->addTab(url, title);
    QCOMPARE(tabModel->count(), count);
    QCOMPARE(countChangeSpy.count(), 1);
    QCOMPARE(currentTabIdChangeSpy.count(), 1);
    QCOMPARE(currentUrlChangedSpy.count(), 1);
    QCOMPARE(tabModel->currentTab()->url(), url);
    QCOMPARE(tabModel->currentTab()->title(), title);

    if (tabModel->rowCount() > 0) {
        QModelIndex index = tabModel->createIndex(0, 0);
        QCOMPARE(tabModel->data(index, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);
    }
}

void tst_declarativetabmodel::activateTabs()
{
    // Original tab order is now reversed.
    QCOMPARE(tabModel->rowCount(), originalTabOrder.count() - 1);

    QCOMPARE(tabModel->currentTab()->url(), originalTabOrder.at(3));
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2));
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(1));
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(0));

    // Active: "http://www.jolla.com" (3) -- original index in brachets
    // "https://sailfishos.org/sailfish-silica/index.html" (2)
    // "file:///opt/tests/sailfish-browser/manual/testpage.html" (1)
    // "http://sailfishos.org" (0)
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));
    QSignalSpy currentUrlChangedSpy(tabModel->currentTab(), SIGNAL(urlChanged()));
    for (int i = 0; i < originalTabOrder.count() - 1; ++i) {
        QString previousActiveUrl = tabModel->currentTab()->url();
        // Activate always last tab
        tabModel->activateTab(2);
        QCOMPARE(currentTabIdChangeSpy.count(), 1);
        QCOMPARE(currentUrlChangedSpy.count(), 1);
        QCOMPARE(tabModel->currentTab()->url(), originalTabOrder.at(i));
        // Previous active url should be pushed to the first model data index
        QModelIndex modelIndex = tabModel->createIndex(0, 0);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);
        currentTabIdChangeSpy.clear();
        currentUrlChangedSpy.clear();
    }

    QString previousActiveUrl = tabModel->currentTab()->url();
    // Activate by url
    tabModel->activateTab(originalTabOrder.at(3));
    QCOMPARE(currentTabIdChangeSpy.count(), 1);
    QCOMPARE(currentUrlChangedSpy.count(), 1);
    // Previous active url should be pushed to the first model data index
    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), previousActiveUrl);

    // Full loop
    QCOMPARE(tabModel->currentTab()->url(), originalTabOrder.at(3));
    currentOrder = modelToStringList(tabModel);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2));
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(1));
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(0));
}

void tst_declarativetabmodel::tabUpdate()
{
    QSignalSpy urlChangedSpy(tabModel->currentTab(), SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tabModel->currentTab(), SIGNAL(titleChanged()));
    QSignalSpy forwardSpy(tabModel->currentTab(), SIGNAL(canGoFowardChanged()));
    QSignalSpy backSpy(tabModel->currentTab(), SIGNAL(canGoBackChanged()));

    int tabId = tabModel->currentTab()->tabId();
    tabModel->updateUrl(tabId, tabModel->currentTab()->url());

    QCOMPARE(urlChangedSpy.count(), 0);
    QCOMPARE(titleChangedSpy.count(), 0);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(backSpy.count(), 0);

    tabModel->updateUrl(tabId, "http://www.jolla.com");
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 0);
    QCOMPARE(forwardSpy.count(), 0);
    QCOMPARE(backSpy.count(), 0);

    // Add title change tests
}


void tst_declarativetabmodel::forwardBackwardNavigation()
{
//    DeclarativeTab *tab = tabModel->currentTab();
//    QSignalSpy urlChangedSpy(tabModel->currentTab(), SIGNAL(urlChanged()));
//    QSignalSpy titleChangedSpy(tabModel->currentTab(), SIGNAL(titleChanged()));
//    QSignalSpy thumbChangedSpy(tabModel->currentTab(), SIGNAL(thumbPathChanged(QString,int)));
//    QSignalSpy forwardSpy(tabModel->currentTab(), SIGNAL(canGoFowardChanged()));
//    QSignalSpy backSpy(tabModel->currentTab(), SIGNAL(canGoBackChanged()));

//    QString url("http://sailfishos.org");
//    tabModel->updateUrl(tab->tabId(), url);

//    QCOMPARE(urlChangedSpy.count(), 1);
//    QCOMPARE(titleChangedSpy.count(), 1);
//    QCOMPARE(forwardSpy.count(), 0);

//    QCOMPARE(tab->url(), url);
//    QVERIFY(tab->title().isEmpty());
//    QVERIFY(tab->thumbnailPath().isEmpty());

//    backSpy.wait();
//    QCOMPARE(backSpy.count(), 1);
//    QVERIFY(tab->canGoBack());

//    QVERIFY(tab->valid());
//    tab->captureScreen(url, 0, 0, 100, 100, 0);
//    thumbChangedSpy.wait();
//    QCOMPARE(thumbChangedSpy.count(), 1);
//    QString path = QString("%1/tab-%2-thumb.png").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).arg(tab->tabId());
//    QCOMPARE(tab->thumbnailPath(), path);

//    QString title("SailfishOS.org");
//    tab->updateUrl(tab->tabId(), url, title);

//    QCOMPARE(urlChangedSpy.count(), 1);
//    QCOMPARE(titleChangedSpy.count(), 2);
//    QCOMPARE(forwardSpy.count(), 0);
//    QCOMPARE(backSpy.count(), 1);
//    QCOMPARE(tab->title(), title);

//    tab->goBack();
//    forwardSpy.wait();
//    QCOMPARE(forwardSpy.count(), 1);
//    QCOMPARE(backSpy.count(), 2);
//    QCOMPARE(urlChangedSpy.count(), 2);
//    QCOMPARE(titleChangedSpy.count(), 3);

//    QVERIFY(!tab->canGoBack());
//    QVERIFY(tab->canGoForward());

//    url = "http://www.jolla.com";
//    title = "Jolla -- we are unlike!";
//    QCOMPARE(tab->url(), url);
//    QCOMPARE(tab->title(), title);

//    // Verify that spy counters will not update (1sec should be enough)
//    QTest::qWait(1000);
//    QCOMPARE(forwardSpy.count(), 1);
//    QCOMPARE(backSpy.count(), 2);
//    QCOMPARE(urlChangedSpy.count(), 2);
//    QCOMPARE(titleChangedSpy.count(), 3);

//    tab->goForward();
//    backSpy.wait();

//    url = "http://sailfishos.org";
//    title = "SailfishOS.org";
//    QCOMPARE(tab->url(), url);
//    QCOMPARE(tab->title(), title);

//    QCOMPARE(forwardSpy.count(), 2);
//    QCOMPARE(backSpy.count(), 3);
//    QCOMPARE(urlChangedSpy.count(), 3);
//    QCOMPARE(titleChangedSpy.count(), 4);

//    QVERIFY(tab->canGoBack());
//    QVERIFY(!tab->canGoForward());

//    forwardSpy.clear();
//    backSpy.clear();
//    urlChangedSpy.clear();
//    titleChangedSpy.clear();
//    tab->goBack();
//    forwardSpy.wait();

//    QCOMPARE(forwardSpy.count(), 1);
//    QCOMPARE(backSpy.count(), 1);
//    QCOMPARE(urlChangedSpy.count(), 1);
//    QCOMPARE(titleChangedSpy.count(), 1);

//    QVERIFY(!tab->canGoBack());
//    QVERIFY(tab->canGoForward());

//    url = "https://sailfishos.org/sailfish-silica/index.html";
//    tab->navigateTo(url);
//    forwardSpy.wait();
//    QCOMPARE(forwardSpy.count(), 2);
//    QCOMPARE(backSpy.count(), 2);
//    QCOMPARE(urlChangedSpy.count(), 2);
//    QCOMPARE(titleChangedSpy.count(), 2);

//    QVERIFY(tab->title().isEmpty());
//    QVERIFY(tab->thumbnailPath().isEmpty());
//    QVERIFY(tab->canGoBack());
//    QVERIFY(!tab->canGoForward());

//    title = "Creating applications Sailfish Silica";
//    tab->updateUrl(url, title);
//    QCOMPARE(forwardSpy.count(), 2);
//    QCOMPARE(backSpy.count(), 2);
//    QCOMPARE(urlChangedSpy.count(), 2);
//    QCOMPARE(titleChangedSpy.count(), 3);
//    QCOMPARE(tab->title(), title);

//    url = "https://sailfishos.org/sailfish-silica/sailfish-silica-introduction.html";
//    tab->navigateTo(url);
//    QCOMPARE(forwardSpy.count(), 2);
//    QCOMPARE(backSpy.count(), 2);
//    QCOMPARE(urlChangedSpy.count(), 3);
//    QCOMPARE(titleChangedSpy.count(), 4);
//    QCOMPARE(tab->url(), url);
//    QVERIFY(tab->title().isEmpty());
//    QVERIFY(tab->canGoBack());
//    QVERIFY(!tab->canGoForward());

//    // Wait and check that all updates have come already
//    QTest::qWait(1000);
//    QCOMPARE(forwardSpy.count(), 2);
//    QCOMPARE(backSpy.count(), 2);
//    QCOMPARE(urlChangedSpy.count(), 3);
//    QCOMPARE(titleChangedSpy.count(), 4);
}

void tst_declarativetabmodel::navigateToInvalidUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("tel") << "tel:+123456798";
    QTest::newRow("sms") << "sms:+123456798";
    QTest::newRow("mailto") << "mailto:joe@example.com";
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1";
    QTest::newRow("geo") << "geo:61.49464,23.77513";
    QTest::newRow("geo://") << "geo://61.49464,23.77513";
}

void tst_declarativetabmodel::navigateToInvalidUrls()
{
    DeclarativeTab *tab = tabModel->currentTab();
    tabModel->updateUrl(tab->tabId(), "http://foobar");

    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));

    QFETCH(QString, url);
    tabModel->updateUrl(tab->tabId(), url);

    QCOMPARE(urlChangedSpy.count(), 0);
    QCOMPARE(titleChangedSpy.count(), 0);
}

void tst_declarativetabmodel::updateInvalidUrls_data()
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

void tst_declarativetabmodel::updateInvalidUrls()
{
    QString expectedUrl = "http://foobar/invalid";
    QString expectedTitle = "Invalid FooBar";
    DeclarativeTab *tab = tabModel->currentTab();
    tabModel->updateUrl(tab->tabId(), expectedUrl/*, expectedTitle*/);

    QFETCH(QString, url);
    QFETCH(QString, title);

    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));

    tabModel->updateUrl(tab->tabId(), url);

    QCOMPARE(urlChangedSpy.count(), 0);
    QCOMPARE(titleChangedSpy.count(), 0);
    QCOMPARE(tab->url(), expectedUrl);
    QCOMPARE(tab->title(), expectedTitle);
}

void tst_declarativetabmodel::updateValidUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::newRow("http") << "http://foobar" << "FooBar";
    QTest::newRow("https") << "https://foobar" << "FooBar 2";
    QTest::newRow("file") << "file://foo/bar/index.html" << "Local foobar";
    QTest::newRow("relative") << "foo/bar/index.html" << "Relative foobar";
}

void tst_declarativetabmodel::updateValidUrls()
{
    QFETCH(QString, url);
    QFETCH(QString, title);

    DeclarativeTab *tab = tabModel->currentTab();
    QSignalSpy urlChangedSpy(tab, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(tab, SIGNAL(titleChanged()));

    tabModel->updateUrl(tab->tabId(), url/*, title*/);

    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 0/*1*/);
    QCOMPARE(tab->url(), url);
    QCOMPARE(tab->title(), title);
}


void tst_declarativetabmodel::remove()
{
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));
    QSignalSpy currentUrlChangedSpy(tabModel->currentTab(), SIGNAL(urlChanged()));
    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));
    QCOMPARE(tabModel->count(), originalTabOrder.count());
    QCOMPARE(tabModel->currentTab()->url(), originalTabOrder.at(3));
    tabModel->remove(1);

    QCOMPARE(currentTabIdChangeSpy.count(), 0);
    QCOMPARE(currentUrlChangedSpy.count(), 0);
    QCOMPARE(tabCountSpy.count(), 1);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabModel->rowCount(), 2);
    QCOMPARE(tabModel->currentTab()->url(), originalTabOrder.at(3));
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2));
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(0));
}

void tst_declarativetabmodel::closeActiveTab()
{
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));
    QSignalSpy currentUrlChangedSpy(tabModel->currentTab(), SIGNAL(urlChanged()));
    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));

    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();

    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabModel->currentTab()->url(), originalTabOrder.at(3));
    tabModel->closeActiveTab();

    QCOMPARE(currentTabIdChangeSpy.count(), 1);
    QCOMPARE(currentUrlChangedSpy.count(), 1);
    QCOMPARE(tabCountSpy.count(), 1);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(tabModel->rowCount(), 1);
    QCOMPARE(tabModel->currentTab()->url(), originalTabOrder.at(2));
    QCOMPARE(tabModel->currentTab()->url(), newActiveUrl);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(0));
}

void tst_declarativetabmodel::invalidTabs_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::newRow("tel") << "tel:+123456798" << "tel";
    QTest::newRow("sms") << "sms:+123456798" << "sms";
    QTest::newRow("mailto") << "mailto:joe@example.com" << "mailto1";
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1" << "mailto2";
}

void tst_declarativetabmodel::invalidTabs()
{
    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy currentTabIdChangeSpy(tabModel, SIGNAL(currentTabIdChanged()));

    QFETCH(QString, url);
    QFETCH(QString, title);
    int originalCount = tabModel->count();
    tabModel->addTab(url, title);

    QCOMPARE(tabModel->count(), originalCount);
    QCOMPARE(countChangeSpy.count(), 0);
    QCOMPARE(currentTabIdChangeSpy.count(), 0);
}

void tst_declarativetabmodel::clear()
{
    QVERIFY(tabModel->count() > 0);
    tabModel->clear();
    QVERIFY(tabModel->count() == 0);
}

QStringList tst_declarativetabmodel::modelToStringList(const DeclarativeTabModel *tabModel) const
{
    QStringList list;
    for (int i = 0; i < tabModel->rowCount(); ++i) {
        QModelIndex modelIndex = tabModel->createIndex(i, 0);
        list << tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
    }
    return list;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_declarativetabmodel testcase;
    qmlRegisterType<DeclarativeTab>("Sailfish.Browser", 1, 0, "Tab");
    qmlRegisterType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel");
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativetabmodel.moc"
