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
#include "declarativewebcontainer.h"

using ::testing::Return;

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   width: 100; height: 100\n" \
        "   property alias tabModel: model\n" \
        "   PersistentTabModel { id: model }\n" \
        "}\n";

struct TabTuple {
    TabTuple(QString url, QString title) : url(url), title(title) {}
    TabTuple() {}

    QString url;
    QString title;
};

Q_DECLARE_METATYPE(TabTuple)

class tst_persistenttabmodel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void addTab_data();
    void addTab();
    void addTabInvalidInput_data();
    void addTabInvalidInput();
    void remove();
    void removeTabById_data();
    void removeTabById();
    void clear();
    void activateTabByUrl_data();
    void activateTabByUrl();
    void activateTabById_data();
    void activateTabById();
    void activateTabByIndex_data();
    void activateTabByIndex();
    void closeActiveTab();
    void updateUrl_data();
    void updateUrl();
    void updateThumbnailPath();
    void onUrlChanged();
    void onTitleChanged();
    void nextActiveTabIndex();
    void roleNames();
    void data_data();
    void data();
    void setUnloaded();
    void newTab();

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
    int argc(0);
    char* argv[0] = {};
    ::testing::InitGoogleMock(&argc, argv);
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


void tst_persistenttabmodel::addTab_data()
{
    QTest::addColumn<QList<TabTuple> >("initialTabs");
    QTest::addColumn<TabTuple>("tabToAdd");
    QTest::addColumn<int>("insertToIndex");

    QList<TabTuple> emptyList;

    QList<TabTuple> list {
        TabTuple(QString("http://example.com"), QString("Test title1")),
        TabTuple(QString("file:///opt/tests/testpage.html"), QString("Test title2")),
        TabTuple(QString("https://example.com"), QString("Test title3"))
    };
    QTest::newRow("append_to_end") << list << TabTuple(QString("http://example2.com"), QString("Test title4")) << 3;
    QTest::newRow("insert_to_start") << list << TabTuple(QString("http://example2.com"), QString("Test title4")) << 0;
    QTest::newRow("insert_to_empty_model") << emptyList << TabTuple(QString("http://example2.com"), QString("Test title4")) << 0;
}

void tst_persistenttabmodel::addTab()
{
    QFETCH(QList<TabTuple>, initialTabs);
    QFETCH(TabTuple, tabToAdd);
    QFETCH(int, insertToIndex);

    // initialize the case
    for (int i = 0; i < initialTabs.count(); i++) {
        tabModel->addTab(initialTabs.at(i).url, initialTabs.at(i).title, i);
    }

    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));
    QSignalSpy nextTabIdChangedSpy(tabModel, SIGNAL(nextTabIdChanged()));
    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));
    QSignalSpy activeTabIndexChangedSpy(tabModel, SIGNAL(activeTabIndexChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    // actual test
    tabModel->addTab(tabToAdd.url, tabToAdd.title, insertToIndex);

    QCOMPARE(countChangeSpy.count(), 1);

    QCOMPARE(tabAddedSpy.count(), 1);
    QList<QVariant> arguments = tabAddedSpy.at(0);
    QCOMPARE(arguments.at(0).toInt(), initialTabs.count() + 1);

    QCOMPARE(nextTabIdChangedSpy.count(), 1);
    QCOMPARE(tabModel->nextTabId(), initialTabs.count() + 2);

    QCOMPARE(activeTabChangedSpy.count(), 1);
    arguments = activeTabChangedSpy.at(0);
    QCOMPARE(arguments.at(0).toInt(), initialTabs.count() + 1);
    QCOMPARE(arguments.at(1).toBool(), true);

    QCOMPARE(tabModel->activeTab().url(), tabToAdd.url);
    QCOMPARE(tabModel->activeTab().title(), tabToAdd.title);

    // when model is not empty two dataChanged signals are emitted;
    int dataChangedCount = initialTabs.count() == 0 ? 1 : 2;
    QCOMPARE(dataChangedSpy.count(), dataChangedCount);

    QCOMPARE(activeTabIndexChangedSpy.count(), 1);

    QModelIndex modelIndex = tabModel->createIndex(insertToIndex, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), tabToAdd.url);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), tabToAdd.title);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), initialTabs.count() + 1);
    QCOMPARE(tabModel->m_tabs.at(insertToIndex).currentLink(), initialTabs.count() + 1);

    if (insertToIndex > 0) {
        modelIndex = tabModel->createIndex(insertToIndex - 1, 0);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), initialTabs.at(insertToIndex - 1).url);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), initialTabs.at(insertToIndex - 1).title);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), insertToIndex);
        QCOMPARE(tabModel->m_tabs.at(insertToIndex - 1).currentLink(), insertToIndex);
    } else if (insertToIndex < initialTabs.count()) {
        modelIndex = tabModel->createIndex(insertToIndex + 1, 0);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), initialTabs.at(insertToIndex).url);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), initialTabs.at(insertToIndex).title);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), insertToIndex + 1);
        QCOMPARE(tabModel->m_tabs.at(insertToIndex + 1).currentLink(), insertToIndex + 1);
    }
}

void tst_persistenttabmodel::addTabInvalidInput_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::newRow("tel") << "tel:+123456798" << "tel";
    QTest::newRow("sms") << "sms:+123456798" << "sms";
    QTest::newRow("mailto") << "mailto:joe@example.com" << "mailto1";
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1" << "mailto2";
}

void tst_persistenttabmodel::addTabInvalidInput()
{
    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    QFETCH(QString, url);
    QFETCH(QString, title);
    tabModel->addTab(url, title, 0);

    QCOMPARE(tabModel->count(), 0);
    QCOMPARE(countChangeSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);
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
    QTest::newRow("ignore_tabId_1")  << 1 << true  << true;
    QTest::newRow("ignore_tabId_2")  << 2 << true  << true;
    QTest::newRow("ignore_tabId_3")  << 2 << true  << true;
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

void tst_persistenttabmodel::activateTabByUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<int>("expectedChanges");
    QTest::addColumn<int>("expectedTabId");
    QTest::addColumn<QString>("expectedUrl");
    QTest::addColumn<QString>("expectedTitle");

    QTest::newRow("url_for_inactive_tab") << QString("file:///opt/tests/testpahe.html") << 1 << 2 << QString("file:///opt/tests/testpahe.html") << QString("Test title2");
    QTest::newRow("url_for_active_tab") << QString("https://example.com") << 0 << 3 << QString("https://example.com") << QString("Test title3");
    QTest::newRow("non_existing_url") << QString("http://some.non.existing.url") << 0 << 3 << QString("https://example.com") << QString("Test title3");
}

void tst_persistenttabmodel::activateTabByUrl()
{
    addThreeTabs();

    QFETCH(QString, url);
    QFETCH(int, expectedChanges);
    QFETCH(int, expectedTabId);
    QFETCH(QString, expectedUrl);
    QFETCH(QString, expectedTitle);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    tabModel->activateTab(url);

    QCOMPARE(tabModel->activeTabId(), expectedTabId);
    QCOMPARE(tabModel->activeTab().url(), expectedUrl);
    QCOMPARE(tabModel->activeTab().title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), expectedChanges);
}

void tst_persistenttabmodel::activateTabById_data()
{
    QTest::addColumn<int>("tabId");
    QTest::addColumn<int>("expectedChanges");
    QTest::addColumn<int>("expectedTabId");
    QTest::addColumn<QString>("expectedUrl");
    QTest::addColumn<QString>("expectedTitle");

    QTest::newRow("inactive_tab") << 2 << 1 << 2 << QString("file:///opt/tests/testpahe.html") << QString("Test title2");
    QTest::newRow("active_tab") << 3 << 0 << 3 << QString("https://example.com") << QString("Test title3");
    QTest::newRow("out_of_range_1") << -1 << 0 << 3 << QString("https://example.com") << QString("Test title3");
    QTest::newRow("out_of_range_2") << 1000 << 0 << 3 << QString("https://example.com") << QString("Test title3");
}

void tst_persistenttabmodel::activateTabById()
{
    addThreeTabs();

    QFETCH(int, tabId);
    QFETCH(int, expectedChanges);
    QFETCH(int, expectedTabId);
    QFETCH(QString, expectedUrl);
    QFETCH(QString, expectedTitle);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    tabModel->activateTabById(tabId);

    QCOMPARE(tabModel->activeTabId(), expectedTabId);
    QCOMPARE(tabModel->activeTab().url(), expectedUrl);
    QCOMPARE(tabModel->activeTab().title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), expectedChanges);
}

void tst_persistenttabmodel::activateTabByIndex_data()
{
    QTest::addColumn<int>("tabIndex");
    QTest::addColumn<int>("expectedChanges");
    QTest::addColumn<int>("expectedTabId");
    QTest::addColumn<QString>("expectedUrl");
    QTest::addColumn<QString>("expectedTitle");

    QTest::newRow("inactive_tab") << 1 << 1 << 2 << QString("file:///opt/tests/testpahe.html") << QString("Test title2");
    QTest::newRow("active_tab") << 2 << 0 << 3 << QString("https://example.com") << QString("Test title3");
    QTest::newRow("out_of_range_1") << -1 << 1 << 1 << QString("http://example.com") << QString("Test title1");
    QTest::newRow("out_of_range_2") << 1000 << 0 << 3 << QString("https://example.com") << QString("Test title3");
}

void tst_persistenttabmodel::activateTabByIndex()
{
    addThreeTabs();

    QFETCH(int, tabIndex);
    QFETCH(int, expectedChanges);
    QFETCH(int, expectedTabId);
    QFETCH(QString, expectedUrl);
    QFETCH(QString, expectedTitle);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    tabModel->activateTab(tabIndex);

    QCOMPARE(tabModel->activeTabId(), expectedTabId);
    QCOMPARE(tabModel->activeTab().url(), expectedUrl);
    QCOMPARE(tabModel->activeTab().title(), expectedTitle);
    QCOMPARE(activeTabChangedSpy.count(), expectedChanges);
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
    QTest::newRow("invalid_url_tel") << 3 << "tel:+123456798" << false << false;
    QTest::newRow("invalid_url_sms") << 3 << "sms:+123456798" << false << false;
    QTest::newRow("invalid_url_mailto_1") << 3 << "mailto:joe@example.com" << false << false;
    QTest::newRow("invalid_url_mailto_2") << 3 << "mailto:joe@example.com?cc=bob@example.com&body=hello1" << false << false;
    QTest::newRow("invalid_url_geo") << 3 << "geo:61.49464,23.77513" << false << false;
    QTest::newRow("invalid_url_geo://") << 3 << "geo://61.49464,23.77513" << false << false;
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

void tst_persistenttabmodel::updateThumbnailPath()
{
    // set up environment
    tabModel->addTab("http://example.com", "initial title", 0);
    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));

    QString path("/path/to/thumbnail");
    tabModel->updateThumbnailPath(1, path);
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(tabModel->m_tabs.at(0).thumbnailPath(), path);
}

void tst_persistenttabmodel::onUrlChanged()
{
    // set up environment
    tabModel->addTab("http://example.com", "initial title", 0);

    DeclarativeWebPage mockPage;
    connect(&mockPage, SIGNAL(urlChanged()), tabModel, SLOT(onUrlChanged()));

    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));
    QSignalSpy tabAddedSpy(tabModel, SIGNAL(tabAdded(int)));

    // 1. a web page is loaded in an existing tab => update model data
    QUrl url("http://newurl.com");
    EXPECT_CALL(mockPage, tabId()).WillOnce(Return(1));
    EXPECT_CALL(mockPage, url()).WillOnce(Return(url));
    EXPECT_CALL(mockPage, initialLoadHasHappened()).WillOnce(Return(true));
    EXPECT_CALL(mockPage, setInitialLoadHasHappened()); // TODO: this call can be optimized out
    emit mockPage.urlChanged();
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(tabAddedSpy.count(), 0);

    // 2. a web page hasn't been loaded yet into a new tab => add tab
    EXPECT_CALL(mockPage, tabId()).WillOnce(Return(2));
    EXPECT_CALL(mockPage, url()).WillOnce(Return(url));
    EXPECT_CALL(mockPage, parentId()).WillOnce(Return(0));
    EXPECT_CALL(mockPage, initialLoadHasHappened()).WillOnce(Return(false));
    EXPECT_CALL(mockPage, setInitialLoadHasHappened());
    emit mockPage.urlChanged();
    QCOMPARE(tabAddedSpy.count(), 1);
    QList<QVariant> arguments = tabAddedSpy.at(0);
    QCOMPARE(arguments.at(0).toInt(), 2);

    // 3. an existing page requested to open another one => add tab next to parent
    EXPECT_CALL(mockPage, tabId()).WillOnce(Return(3));
    EXPECT_CALL(mockPage, url()).WillOnce(Return(url));
    EXPECT_CALL(mockPage, parentId()).WillOnce(Return(1));
    EXPECT_CALL(mockPage, initialLoadHasHappened()).WillOnce(Return(false));
    EXPECT_CALL(mockPage, setInitialLoadHasHappened());
    emit mockPage.urlChanged();
    QCOMPARE(tabAddedSpy.count(), 2);
    arguments = tabAddedSpy.at(1);
    QCOMPARE(arguments.at(0).toInt(), 3);
}

void tst_persistenttabmodel::onTitleChanged()
{
    // set up environment
    tabModel->addTab("http://example.com", "initial title", 0);

    DeclarativeWebPage mockPage;
    connect(&mockPage, SIGNAL(titleChanged()), tabModel, SLOT(onTitleChanged()));

    QSignalSpy dataChangedSpy(tabModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)));

    EXPECT_CALL(mockPage, tabId()).WillOnce(Return(1));
    QUrl url;
    EXPECT_CALL(mockPage, url()).WillOnce(Return(url));
    EXPECT_CALL(mockPage, title()).WillOnce(Return(QString("Hello world")));
    emit mockPage.titleChanged();

    QCOMPARE(dataChangedSpy.count(), 1);
}

void tst_persistenttabmodel::nextActiveTabIndex()
{
    DeclarativeWebContainer container;
    DeclarativeWebPage page;
    tabModel->setWebContainer(&container);

    EXPECT_CALL(container, webPage()).WillRepeatedly(Return(&page));
    EXPECT_CALL(page, parentId()).WillOnce(Return(1));
    EXPECT_CALL(page, tabId()).WillOnce(Return(2));
    tabModel->nextActiveTabIndex(0);
}

void tst_persistenttabmodel::roleNames()
{
    // Here we test the method doesn't explode when called.
    tabModel->roleNames();
}

void tst_persistenttabmodel::data_data()
{
    QTest::addColumn<QModelIndex>("index");
    QTest::addColumn<int>("role");
    QTest::addColumn<bool>("isValid");

    QModelIndex modelIndex = tabModel->createIndex(-1, 0);
    QTest::newRow("invalid_index_1") << modelIndex << (int)DeclarativeTabModel::TabIdRole << false;
    modelIndex = tabModel->createIndex(1000, 0);
    QTest::newRow("invalid_index_2") << modelIndex << (int)DeclarativeTabModel::TabIdRole << false;
    modelIndex = tabModel->createIndex(1, 0);
    QTest::newRow("TabIdRole") << modelIndex  << (int)DeclarativeTabModel::TabIdRole << true;
    QTest::newRow("ActiveRole") << modelIndex  << (int)DeclarativeTabModel::ActiveRole << true;
    QTest::newRow("UrlRole") << modelIndex  << (int)DeclarativeTabModel::UrlRole << true;
    QTest::newRow("TitleRole") << modelIndex  << (int)DeclarativeTabModel::TitleRole << true;
    QTest::newRow("ThumbPathRole") << modelIndex  << (int)DeclarativeTabModel::ThumbPathRole << true;
    QTest::newRow("InvalidRole") << modelIndex  << -10000 << false;
}

void tst_persistenttabmodel::data()
{
    // Set up environment
    addThreeTabs();

    QFETCH(QModelIndex, index);
    QFETCH(int, role);
    QFETCH(bool, isValid);

    QVariant data = tabModel->data(index, role);
    if (!isValid) {
        QCOMPARE(data, QVariant());
    } else if (role == DeclarativeTabModel::UrlRole) {
        QCOMPARE(data.toString(), QString("file:///opt/tests/testpahe.html"));
    } else if (role == DeclarativeTabModel::TitleRole) {
        QCOMPARE(data.toString(), QString("Test title2"));
    } else if (role == DeclarativeTabModel::TabIdRole) {
        QCOMPARE(data.toInt(), 2);
    } else if (role == DeclarativeTabModel::ActiveRole) {
        QCOMPARE(data.toBool(), false);
    }
}

void tst_persistenttabmodel::setUnloaded()
{
    QSignalSpy loadedChangedSpy(tabModel, SIGNAL(loadedChanged()));
    tabModel->setUnloaded();
    QCOMPARE(loadedChangedSpy.count(), 1);
}

void tst_persistenttabmodel::newTab()
{
    QSignalSpy newTabRequestedSpy(tabModel, SIGNAL(newTabRequested(QString, QString, int)));
    tabModel->newTab(QString(), QString(), 0);
    QCOMPARE(newTabRequestedSpy.count(), 1);
    QCOMPARE(tabModel->waitingForNewTab(), true);
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
