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
