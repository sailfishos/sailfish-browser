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

#include "declarativewebcontainer.h"
#include "persistenttabmodel.h"
#include "dbmanager.h"
#include "testobject.h"

static const QByteArray QML_SNIPPET = \
        "import QtQuick 2.0\n" \
        "import Sailfish.Browser 1.0\n" \
        "Item {\n" \
        "   width: 100; height: 100\n" \
        "   property alias tabModel: model\n" \
        "   PersistentTabModel { id: model }\n" \
        "}\n";

class tst_declarativetabmodel : public TestObject
{
    Q_OBJECT

public:
    tst_declarativetabmodel();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void validTabs_data();
    void validTabs();

    void activateTabs();

    void remove();
    void closeActiveTab();

    void multipleTabsWithSameUrls();

    void updateInvalidUrls_data();
    void updateInvalidUrls();

    void updateValidUrls_data();
    void updateValidUrls();

    void invalidTabs_data();
    void invalidTabs();

    void reloadModel();
    void changeTabAndLoad();
    void insertTab();

    void clear();

private:
    QStringList modelToStringList(const DeclarativeTabModel *tabModel) const;

    int currentTabId() const;
    QString currentTabUrl() const;

    DeclarativeTabModel *tabModel;
    DeclarativeWebContainer *webContainer;
    QList<TestTab> originalTabOrder;
};

tst_declarativetabmodel::tst_declarativetabmodel()
    : TestObject(QML_SNIPPET)
{
    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    webContainer = new DeclarativeWebContainer(this);
    tabModel->setWebContainer(webContainer);

    originalTabOrder.append(TestTab("http://sailfishos.org", "SailfishOS.org"));
    originalTabOrder.append(TestTab("file:///opt/tests/sailfish-browser/manual/testpage.html", "Test Page"));
    originalTabOrder.append(TestTab("https://sailfishos.org/sailfish-silica/index.html", "Creating applications with Sailfish Silica | Sailfish Silica 1.0"));
    originalTabOrder.append(TestTab("http://www.jolla.com", "Jolla -- we are unlike!"));
}

void tst_declarativetabmodel::initTestCase()
{
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
        QTest::newRow(QString("%1-tab").arg(i+1).toLatin1()) << originalTabOrder.at(i).url << originalTabOrder.at(i).title << i+1;
    }
}

void tst_declarativetabmodel::validTabs()
{
    QSignalSpy countChangeSpy(tabModel, SIGNAL(countChanged()));

    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(int, count);

    tabModel->addTab(url, title, tabModel->count());
    QCOMPARE(tabModel->count(), count);
    QCOMPARE(countChangeSpy.count(), 1);
    QCOMPARE(tabModel->activeTab().url(), url);
    QCOMPARE(tabModel->activeTab().title(), title);

    for (int i = 0; i < count; ++i) {
        QModelIndex index = tabModel->createIndex(i, 0);
        QCOMPARE(tabModel->data(index, DeclarativeTabModel::UrlRole).toString(), originalTabOrder.at(i).url);
    }
}

void tst_declarativetabmodel::activateTabs()
{
    QCOMPARE(tabModel->rowCount(), originalTabOrder.count());

    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(3).url);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(0).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(1).url);
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(3), originalTabOrder.at(3).url);

    // "http://sailfishos.org"
    // "file:///opt/tests/sailfish-browser/manual/testpage.html"
    // "https://sailfishos.org/sailfish-silica/index.html"
    // "http://www.jolla.com"
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));
    for (int i = 0; i < originalTabOrder.count(); ++i) {
        tabModel->activateTab(i, true);
        QCOMPARE(activeTabChangedSpy.count(), 1);
        QString expectedUrl = originalTabOrder.at(i).url;
        QCOMPARE(tabModel->activeTab().url(), expectedUrl);

        QModelIndex modelIndex = tabModel->createIndex(i, 0);
        QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), expectedUrl);
        activeTabChangedSpy.clear();
    }

    // Activate by url, last tab is currently active.
    QString activateUrl = originalTabOrder.at(0).url;
    tabModel->activateTab(activateUrl);
    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(tabModel->activeTab().url(), activateUrl);
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(0).url);
    currentOrder = modelToStringList(tabModel);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(0).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(1).url);
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(3), originalTabOrder.at(3).url);
}

void tst_declarativetabmodel::remove()
{
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));
    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));
    QCOMPARE(tabModel->count(), originalTabOrder.count());
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(0).url);
    tabModel->remove(1);
    // Active tab should not change.
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(0).url);

    QCOMPARE(activeTabChangedSpy.count(), 0);
    QCOMPARE(tabCountSpy.count(), 1);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabModel->rowCount(), 3);
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(0).url);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(0).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(2), originalTabOrder.at(3).url);
}

void tst_declarativetabmodel::closeActiveTab()
{
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));
    QSignalSpy tabCountSpy(tabModel, SIGNAL(countChanged()));

    // 2nd tab will be the new active tab. 1st tab is currently active.
    // Original zero is active tab ("http://sailfishos.org")
    QModelIndex modelIndex = tabModel->createIndex(1, 0);
    QString newActiveUrl = tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString();
    QCOMPARE(newActiveUrl, originalTabOrder.at(2).url);

    QCOMPARE(tabModel->count(), 3);
    QCOMPARE(tabModel->activeTab().url(), originalTabOrder.at(0).url);
    tabModel->closeActiveTab();

    QCOMPARE(activeTabChangedSpy.count(), 1);
    QCOMPARE(tabCountSpy.count(), 1);
    QStringList currentOrder = modelToStringList(tabModel);
    QCOMPARE(tabModel->count(), 2);
    QCOMPARE(tabModel->rowCount(), 2);
    QCOMPARE(tabModel->activeTab().url(), newActiveUrl);
    QCOMPARE(currentOrder.at(0), originalTabOrder.at(2).url);
    QCOMPARE(currentOrder.at(1), originalTabOrder.at(3).url);
}

void tst_declarativetabmodel::multipleTabsWithSameUrls()
{
    QString page1Tab1Url = "http://www.foobar.com/page1";
    QString page1Tab1Title = "First Page";
    // tab1: page1 ("First Page") and page2 ("")
    tabModel->addTab(page1Tab1Url, page1Tab1Title, tabModel->count());
    int tab1 = currentTabId();
    QCOMPARE(tabModel->activeTab().url(), page1Tab1Url);
    QCOMPARE(tabModel->activeTab().title(), page1Tab1Title);

    QTest::qWait(1000);

    QString page2Tab1Url = "http://www.foobar.com/page2";
    tabModel->updateUrl(tab1, page2Tab1Url, false);
    QTest::qWait(1000);
    QCOMPARE(tabModel->activeTab().url(), page2Tab1Url);
    // Title is cleared in the model when the url changes.
    QVERIFY(tabModel->activeTab().title().isEmpty());

    QTest::qWait(1000);

    // tab2: page1 ("First Page Too") and page2 ("Second Page Too")
    QString page1Tab2Url = page1Tab1Url;
    QString page1Tab2Title = "First Page Too";
    tabModel->addTab(page1Tab2Url, page1Tab2Title, tabModel->count());
    int tab2 = currentTabId();
    QVERIFY(tab1 != tab2);
    QCOMPARE(tabModel->activeTab().url(), page1Tab2Url);
    QCOMPARE(tabModel->activeTab().title(), page1Tab2Title);
    QTest::qWait(1000);

    int index = tabModel->findTabIndex(tab1);
    QModelIndex modelIndex = tabModel->createIndex(index, 0);

    // tab1 has page2Tab1Url and empty title still.
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), page2Tab1Url);
    QVERIFY(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString().isEmpty());

    QTest::qWait(1000);

    QString page2Tab2Url = page2Tab1Url;
    QString page2Tab2Title = "Second Page Too";
    tabModel->updateUrl(tab2, page2Tab2Url, false);

    QTest::qWait(1000);

    QCOMPARE(tabModel->activeTab().url(), page2Tab2Url);
    QVERIFY(tabModel->activeTab().title().isEmpty());

    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), page2Tab1Url);
    QVERIFY(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString().isEmpty());

    QTest::qWait(1000);

    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));
    int expectedCount = tabModel->count() - 2;
    tabModel->removeTabById(tab1, true);
    waitSignals(activeTabChangedSpy, 1);
    tabModel->removeTabById(tab2, true);
    waitSignals(activeTabChangedSpy, 2);
    QVERIFY(tabModel->count() == expectedCount);
}

void tst_declarativetabmodel::updateInvalidUrls_data()
{
    QString expectedUrl = tabModel->activeTab().url();
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expectedUrl");
    QTest::newRow("tel") << "tel:+123456798" << expectedUrl;
    QTest::newRow("sms") << "sms:+123456798" << expectedUrl;
    QTest::newRow("mailto") << "mailto:joe@example.com" << expectedUrl;
    QTest::newRow("mailto query does not count") << "mailto:joe@example.com?cc=bob@example.com&body=hello1" << expectedUrl;
    QTest::newRow("geo") << "geo:61.49464,23.77513" << expectedUrl;
    QTest::newRow("geo://") << "geo://61.49464,23.77513" << expectedUrl;
}

void tst_declarativetabmodel::updateInvalidUrls()
{
    QFETCH(QString, expectedUrl);
    QFETCH(QString, url);
    tabModel->updateUrl(currentTabId(), url, false);
    QTest::qWait(1000);
    QCOMPARE(tabModel->activeTab().url(), expectedUrl);
}

void tst_declarativetabmodel::updateValidUrls_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("http") << "http://foobar";
    QTest::newRow("https") << "https://foobar";
    QTest::newRow("file") << "file://foo/bar/index.html";
    QTest::newRow("relative") << "foo/bar/index.html";
}

void tst_declarativetabmodel::updateValidUrls()
{
    QFETCH(QString, url);

    int tabId = currentTabId();
    tabModel->updateUrl(tabId, url, false);
    QCOMPARE(tabModel->activeTab().url(), url);
    QTest::qWait(1000);
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
    QSignalSpy activeTabChangedSpy(tabModel, SIGNAL(activeTabChanged(int,bool)));

    QFETCH(QString, url);
    QFETCH(QString, title);
    int originalCount = tabModel->count();
    tabModel->addTab(url, title, originalCount);

    QCOMPARE(tabModel->count(), originalCount);
    QCOMPARE(countChangeSpy.count(), 0);
    QCOMPARE(activeTabChangedSpy.count(), 0);
}

void tst_declarativetabmodel::reloadModel()
{
    tabModel->deleteLater();
    QTest::waitForEvents();

    setTestData(EMPTY_QML);
    setTestData(QML_SNIPPET);

    tabModel = TestObject::qmlObject<DeclarativeTabModel>("tabModel");
    QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
    QVERIFY(tabModel);
    waitSignals(loadedSpy, 1);

    QCOMPARE(tabModel->count(), 2);

    int activeTabIndex = tabModel->findTabIndex(tabModel->activeTab().tabId());
    QModelIndex modelIndex = tabModel->createIndex(activeTabIndex, 0);
    QCOMPARE(tabModel->activeTab().tabId(), tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt());
    QCOMPARE(tabModel->activeTab().url(), tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString());
    QCOMPARE(tabModel->activeTab().title(), tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString());
    QCOMPARE(tabModel->activeTab().currentLink(), tabModel->m_tabs.at(activeTabIndex).currentLink());

    modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("https://sailfishos.org/sailfish-silica/index.html"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("Creating applications with Sailfish Silica | Sailfish Silica 1.0"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 3);
    QCOMPARE(tabModel->m_tabs.at(0).currentLink(), 3);

    modelIndex = tabModel->createIndex(1, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("foo/bar/index.html"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 4);
    QCOMPARE(tabModel->m_tabs.at(1).currentLink(), 12);
}

void tst_declarativetabmodel::changeTabAndLoad()
{
    // Highest linkId of available tabs is 13
    int nextLinkId = DBManager::instance()->nextLinkId();
    QCOMPARE(nextLinkId, 13);

    tabModel->activateTab(1);
    QCOMPARE(currentTabId(), 4);

    QCOMPARE(tabModel->activeTab().currentLink(), 12);
    QString url = "http://www.foobar.com/something";
    tabModel->updateUrl(currentTabId(), url, false);
    QTest::qWait(1000);

    QCOMPARE(tabModel->activeTab().tabId(), 4);
    QCOMPARE(tabModel->activeTab().currentLink(), nextLinkId);
    QCOMPARE(tabModel->activeTab().url(), url);
    QCOMPARE(tabModel->activeTab().title(), QString(""));
}

void tst_declarativetabmodel::insertTab()
{
    QString url("http://foobar/tab2");
    QString title("tab2");
    int oldCount = tabModel->count();
    QVERIFY(oldCount == 2);
    tabModel->addTab(url, title, 1);

    QModelIndex modelIndex = tabModel->createIndex(0, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("https://sailfishos.org/sailfish-silica/index.html"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("Creating applications with Sailfish Silica | Sailfish Silica 1.0"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 3);
    QCOMPARE(tabModel->m_tabs.at(0).currentLink(), 3);

    modelIndex = tabModel->createIndex(1, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("http://foobar/tab2"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString("tab2"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 7);
    QCOMPARE(tabModel->m_tabs.at(1).currentLink(), 14);

    modelIndex = tabModel->createIndex(2, 0);
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::UrlRole).toString(), QString("http://www.foobar.com/something"));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TitleRole).toString(), QString(""));
    QCOMPARE(tabModel->data(modelIndex, DeclarativeTabModel::TabIdRole).toInt(), 4);
    QCOMPARE(tabModel->m_tabs.at(2).currentLink(), 13);
    QVERIFY(oldCount+1 == tabModel->count());
}

void tst_declarativetabmodel::clear()
{
    QVERIFY(tabModel->count() > 0);
    tabModel->clear();
    QVERIFY(tabModel->count() == 0);
    tabModel->deleteLater();
    QTest::waitForEvents();
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

int tst_declarativetabmodel::currentTabId() const
{
    if (tabModel->activeTab().isValid()) {
        return tabModel->activeTab().tabId();
    }
    return 0;
}

QString tst_declarativetabmodel::currentTabUrl() const
{
    if (tabModel->activeTab().isValid()) {
        return tabModel->activeTab().url();
    }
    return QString();
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    qmlRegisterUncreatableType<DeclarativeTabModel>("Sailfish.Browser", 1, 0, "TabModel", "TabModel is abstract!");
    qmlRegisterType<PersistentTabModel>("Sailfish.Browser", 1, 0, "PersistentTabModel");
    tst_declarativetabmodel testcase;
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativetabmodel.moc"
