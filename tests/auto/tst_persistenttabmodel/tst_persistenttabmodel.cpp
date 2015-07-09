/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest/QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickView>

#include "persistenttabmodel.h"
#include "dbmanager.h"
#include "declarativewebpage.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   width: 100; height: 100\n" \
        "   property alias tabModel: model\n" \
        "   PersistentTabModel { id: model }\n" \
        "}\n";

class tst_persistenttabmodel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void addTab();
    void remove();
    void removeTabById_data();
    void removeTabById();
    void clear();
    void activateTab_by_url_data();
    void activateTab_by_url();
    void activateTabById_data();
    void activateTabById();
    void closeActiveTab();
    void updateUrl_data();
    void updateUrl();
    void onTitleChanged();

private:
    void addThreeTabs();

    PersistentTabModel* tabModel;
    QQuickView mView;
    QPointer<QQmlComponent> mComponent;
    QObject* mRootObject;
    QString mDbFile;
};

void tst_persistenttabmodel::initTestCase()
{
    qGuiApp->setAttribute(Qt::AA_Use96Dpi, true);
    qmlRegisterUncreatableType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel",
                                                    "TabModel is abstract!");
    qmlRegisterType<PersistentTabModel>("Sailfish.Browser", 1, 0, "PersistentTabModel");
    mComponent = new QQmlComponent(mView.engine());
    mComponent->setData(QML_SNIPPET, QUrl());
    mDbFile = QString("%1/%2")
            .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
            .arg(QLatin1String(DB_NAME));
}

void tst_persistenttabmodel::cleanupTestCase()
{
    delete mComponent;
}

void tst_persistenttabmodel::init()
{
    mRootObject = mComponent->create(mView.engine()->rootContext());
    QVariant var = mRootObject->property("tabModel");
    tabModel = qobject_cast<PersistentTabModel*>(qvariant_cast<QObject*>(var));
    QVERIFY(tabModel);

    if (!tabModel->loaded()) {
        QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
        // Tabs must be loaded with in 500ms
        QVERIFY(loadedSpy.wait());
        QCOMPARE(loadedSpy.count(), 1);
    }
}

void tst_persistenttabmodel::cleanup()
{
    delete mRootObject;
    delete DBManager::instance();
    QFile dbFile(mDbFile);
    QVERIFY(dbFile.remove());
}

void tst_persistenttabmodel::addTab()
{
    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));
    QSignalSpy nextTabIdChangedSpy(tabModel, SIGNAL(nextTabIdChanged()));
    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));
    QSignalSpy activeTabIndexChangedSpy(tabModel, SIGNAL(activeTabIndexChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    QList<QString> urls, titles;
    urls << "http://example.com" << "file:///opt/tests/testpage.html" << "https://example.com";
    titles << "Test title1" << "Test title2" << "Test title3";

    QList<QVariant> arguments;

    // append tab
    for (int i = 0; i < urls.count(); i++) {
        tabModel->addTab(urls.at(i), titles.at(i), tabModel->count());
        QCOMPARE(countChangeSpy.count(), i+1);

        QCOMPARE(tabAddedSpy.count(), i+1);
        arguments = tabAddedSpy.at(i);
        QCOMPARE(arguments.at(0).toInt(), i+1);

        QCOMPARE(nextTabIdChangedSpy.count(), i+1);
        QCOMPARE(tabModel->nextTabId(), i+2);
        // when model is not empty two dataChanged signals are emitted;
        int dataChangedCount = i == 0 ? 1 : (i * 2) + 1;
        QCOMPARE(dataChangedSpy.count(), dataChangedCount);
        QCOMPARE(activeTabIndexChangedSpy.count(), i+1);

        QCOMPARE(activeTabChangedSpy.count(), i+1);
        arguments = activeTabChangedSpy.at(i);
        QCOMPARE(arguments.at(0).toInt(), i+1);
        QCOMPARE(arguments.at(1).toBool(), true);

        QCOMPARE(tabModel->activeTab().url(), urls.at(i));
        QCOMPARE(tabModel->activeTab().title(), titles.at(i));
    }

    // insert tab
    tabModel->addTab(QString("http://inserted.tab"), QString("Inserted tab"), 1);
    QCOMPARE(countChangeSpy.count(), 4);
    QCOMPARE(tabAddedSpy.count(), 4);
    arguments = tabAddedSpy.at(3);
    QCOMPARE(arguments.at(0).toInt(), 4);
    QCOMPARE(activeTabChangedSpy.count(), 4);
    arguments = activeTabChangedSpy.at(3);
    QCOMPARE(arguments.at(0).toInt(), 4);
    QCOMPARE(arguments.at(1).toBool(), true);

    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("http://example.com"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("Test title1"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 1);
    QCOMPARE(tabModel->m_tabs.at(0).currentLink(), 1);

    modelIndex = tabModel->createIndex(1, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("http://inserted.tab"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("Inserted tab"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 4);
    QCOMPARE(tabModel->m_tabs.at(1).currentLink(), 4);

    modelIndex = tabModel->createIndex(2, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("file:///opt/tests/testpage.html"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("Test title2"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 2);
    QCOMPARE(tabModel->m_tabs.at(2).currentLink(), 2);
}

void tst_persistenttabmodel::remove()
{
    addThreeTabs();

    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));
    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));
    QSignalSpy activeTabIndexChangedSpy(tabModel, SIGNAL(activeTabIndexChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    // nothing should happen
    tabModel->remove(-1);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabClosedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 0);
    QCOMPARE(activeTabIndexChangedSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);

    // nothing should happen
    tabModel->remove(10);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabClosedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 0);
    QCOMPARE(activeTabIndexChangedSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);

    // removing inactive tab
    tabModel->remove(1);
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(tabClosedSpy.count(), 1);
    QCOMPARE(activeTabIndexChangedSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);
    QCOMPARE(dataChangedSpy.count(), 0);

    // remove active tab
    tabModel->remove(tabModel->count() - 1);
    QCOMPARE(tabModel->count(), 1);
    QCOMPARE(tabClosedSpy.count(), 2); // by now two tabs have been closed
    QCOMPARE(activeTabIndexChangedSpy.count(), 1);
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.count(), 1);
}

void tst_persistenttabmodel::removeTabById_data()
{
    QTest::addColumn<int>("tabId");
    QTest::addColumn<bool>("activeTab");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("first_tab")       << 1 << false << true;
    QTest::newRow("middle_tab")      << 2 << false << true;
    QTest::newRow("last_tab")        << 3 << false << true;
    QTest::newRow("igmore_tabId_1")  << 1 << true  << true;
    QTest::newRow("igmore_tabId_2")  << 2 << true  << true;
    QTest::newRow("igmore_tabId_3")  << 2 << true  << true;
    QTest::newRow("invalid_tabId_1") << 0 << false << false;
    QTest::newRow("invalid_tabId_2") << 9 << false << false;
    QTest::newRow("invalid_tabId_3") << -1 << false << false;
}

void tst_persistenttabmodel::removeTabById()
{
    addThreeTabs();

    QFETCH(int, tabId);
    QFETCH(bool, activeTab);
    QFETCH(bool, isValid);

    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    tabModel->removeTabById(tabId, activeTab);

    if (isValid) {
        QCOMPARE(tabClosedSpy.count(), 1);

        if (activeTab) {
            QCOMPARE(activeTabChangedSpy.count(), 1);
        }
    } else {
        QCOMPARE(tabClosedSpy.count(), 0);
    }
}

void tst_persistenttabmodel::clear()
{
    addThreeTabs();
    QSignalSpy tabClosedSpy(tabModel, SIGNAL(tabClosed(int)));

    tabModel->clear();

    QCOMPARE(tabClosedSpy.count(), 3);
    QCOMPARE(tabModel->count(), 0);
}

void tst_persistenttabmodel::activateTab_by_url_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("isExpectedToActivate");

    QTest::newRow("url_for_inactive_tab") << QString("file:///opt/tests/testpahe.html") << true;
    QTest::newRow("url_for_active_tab") << QString("https://example.com") << false;
    QTest::newRow("non_existing_url") << QString("http://some.non.existing.url") << false;
}

void tst_persistenttabmodel::activateTab_by_url()
{
    addThreeTabs();

    QFETCH(QString, url);
    QFETCH(bool, isExpectedToActivate);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    int oldActiveTabId = tabModel->activeTabId();
    tabModel->activateTab(url);

    if (isExpectedToActivate) {
        QCOMPARE(activeTabChangedSpy.count(), 1);
        QVERIFY(tabModel->activeTabId() != oldActiveTabId);
    } else {
        QCOMPARE(activeTabChangedSpy.count(), 0);
        QCOMPARE(tabModel->activeTabId(), oldActiveTabId);
    }
}

void tst_persistenttabmodel::activateTabById_data()
{
    QTest::addColumn<int>("tabId");
    QTest::addColumn<bool>("isExpectedToActivate");

    QTest::newRow("inactive_tab")   << 2    << true;
    QTest::newRow("active_tab")     << 3    << false;
    QTest::newRow("out_of_range_1") << -1   << false;
    QTest::newRow("out_of_range_2") << 1000 << false;
}

void tst_persistenttabmodel::activateTabById()
{
    addThreeTabs();

    QFETCH(int, tabId);
    QFETCH(bool, isExpectedToActivate);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    int oldActiveTabId = tabModel->activeTabId();
    tabModel->activateTabById(tabId);

    if (isExpectedToActivate) {
        QCOMPARE(activeTabChangedSpy.count(), 1);
        QCOMPARE(tabModel->activeTabId(), tabId);
    } else {
        QCOMPARE(activeTabChangedSpy.count(), 0);
        QCOMPARE(tabModel->activeTabId(), oldActiveTabId);
    }
}

void tst_persistenttabmodel::closeActiveTab()
{
    addThreeTabs();
    // make the first tab active
    tabModel->activateTabById(1);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));
    QSignalSpy tabCountChangedSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy activeTabIndexChangedSpy(tabModel, SIGNAL(activeTabIndexChanged()));

    tabModel->closeActiveTab();

    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(tabCountChangedSpy.count(), 1);
    // TODO: this is wrong. Fix DeclarativeTabModel not to emit this signal.
    QCOMPARE(activeTabIndexChangedSpy.count(), 1);
    QCOMPARE(tabModel->activeTabIndex(), 0);
    QCOMPARE(tabModel->activeTabId(), 2);
    QCOMPARE(tabModel->count(), 2);
}

void tst_persistenttabmodel::updateUrl_data()
{
    QTest::addColumn<int>("tabId");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("initialLoad");
    QTest::addColumn<bool>("isExpectedToUpdate");

    QTest::newRow("update_inactive_tab_http") << 1 << "http://some.real.site" << false << true;
    QTest::newRow("update_inactive_tab_https") << 1 << "https://some.real.site" << false << true;
    QTest::newRow("update_inactive_tab_file") << 1 << "file:///foo/bar/index.html" << false << true;
    QTest::newRow("update_inactive_tab_relative") << 1 << "foo/bar/index.html" << false << true;
    QTest::newRow("update_active_tab") << 3 << "http://some.real.site" << false << true;
    QTest::newRow("invalide_url_tel") << 3 << "tel:+123456798" << false << false;
    QTest::newRow("invalide_url_sms") << 3 << "sms:+123456798" << false << false;
    QTest::newRow("invalide_url_mailto_1") << 3 << "mailto:joe@example.com" << false << false;
    QTest::newRow("invalide_url_mailto_2") << 3 << "mailto:joe@example.com?cc=bob@example.com&body=hello1" << false << false;
    QTest::newRow("invalide_url_geo") << 3 << "geo:61.49464,23.77513" << false << false;
    QTest::newRow("invalide_url_geo://") << 3 << "geo://61.49464,23.77513" << false << false;
}

void tst_persistenttabmodel::updateUrl()
{
    addThreeTabs();

    QFETCH(int, tabId);
    QFETCH(QString, url);
    QFETCH(bool, initialLoad);
    QFETCH(bool, isExpectedToUpdate);

    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));

    tabModel->updateUrl(tabId, url, initialLoad);

    if (isExpectedToUpdate) {
        QCOMPARE(dataChangedSpy.count(), 1);
        Tab tab = tabModel->tabs().at(tabModel->findTabIndex(tabId));
        QCOMPARE(tab.url(), url);
        QCOMPARE(tab.title(), QString(""));
        QCOMPARE(tab.currentLink(), 4);
    } else {
        QCOMPARE(dataChangedSpy.count(), 0);
    }
}

void tst_persistenttabmodel::onTitleChanged()
{
    // set up environment
    tabModel->addTab("http://example.com", "initial title", 0);

    DeclarativeWebPage mockPage;
    connect(&mockPage, SIGNAL(titleChanged()), tabModel, SLOT(onTitleChanged()));

    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));

    mockPage.m_tabId = 1;
    mockPage.setTitle("Hello world");
    QCOMPARE(dataChangedSpy.count(), 1);
}

void tst_persistenttabmodel::addThreeTabs()
{
    QList<QString> urls, titles;
    urls << "http://example.com" << "file:///opt/tests/testpahe.html" << "https://example.com";
    titles << "Test title1" << "Test title2" << "Test title3";

    for (int i = 0; i < urls.count(); i++) {
        tabModel->addTab(urls.at(i), titles.at(i), tabModel->count());
    }
}

QTEST_MAIN(tst_persistenttabmodel)
#include "tst_persistenttabmodel.moc"
