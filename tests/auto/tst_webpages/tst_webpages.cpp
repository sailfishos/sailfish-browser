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

#include "webpagefactory.h"
#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "tab.h"

#include "webpages.h"

Q_DECLARE_METATYPE(QList<Tab>)
Q_DECLARE_METATYPE(Tab)

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;

class tst_webpages : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void initialize();
    void count();
    void setMaxLivePages();
    void maxLivePages();
    void alive();
    void page_data();
    void page();
    void pageWithBrokenPageFactory();
    void release_data();
    void release();
    void clear_data();
    void clear();
    void parentTabId_data();
    void parentTabId();

private:
    WebPages* m_webPages;
    WebPageFactory m_pageFactory;
};

void tst_webpages::initTestCase()
{
    QMozContext::GetInstance();
}

void tst_webpages::cleanupTestCase()
{
    delete QMozContext::GetInstance();
}

void tst_webpages::init()
{
    m_webPages = new WebPages(&m_pageFactory);
}

void tst_webpages::cleanup()
{
    delete m_webPages;
}

void tst_webpages::initialize()
{
    EXPECT_CALL(*QMozContext::GetInstance(), setPixelRatio(_));
    DeclarativeWebContainer webContainer;
    webContainer.setForeground(true);

    QVERIFY(!m_webPages->initialized());
    m_webPages->initialize(&webContainer);
    QVERIFY(m_webPages->initialized());

    // check that the connection with ::updateBackgroundTimestamp() has been created
    webContainer.setForeground(false);
    QVERIFY(m_webPages->m_backgroundTimestamp > 0);
}

void tst_webpages::count()
{
    EXPECT_CALL(*QMozContext::GetInstance(), setPixelRatio(_));
    DeclarativeWebContainer webContainer;
    m_webPages->initialize(&webContainer);

    QCOMPARE(m_webPages->count(), 0);

    DeclarativeWebPage* page = new DeclarativeWebPage();
    EXPECT_CALL(m_pageFactory, createWebPage(_, _, _)).WillOnce(Return(page));

    // add one page and check count()
    EXPECT_CALL(*page, tabId());
    EXPECT_CALL(*page, uniqueID());
    EXPECT_CALL(*page, parentId());
    EXPECT_CALL(*page, completed());
    m_webPages->page(Tab(1, "http://example.com", "Test title", ""));
    QCOMPARE(m_webPages->count(), 1);
}

void tst_webpages::setMaxLivePages()
{
    m_webPages->setMaxLivePages(171);
    QCOMPARE(m_webPages->m_activePages.maxLivePages(), 171);
}

void tst_webpages::maxLivePages()
{
    QCOMPARE(m_webPages->maxLivePages(), 5);
}

void tst_webpages::alive()
{
    QCOMPARE(m_webPages->alive(1), false);
}

void tst_webpages::page_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");
    QTest::addColumn<Tab>("tab");
    QTest::addColumn<int>("parentId");
    QTest::addColumn<int>("maxLiveTabs");
    QTest::addColumn<bool>("isPageCreationExpected");

    QList<Tab> noInitialTabs;
    QList<Tab> threeTabs {
        Tab(1, "http://example1.com", "Title1", ""),
        Tab(2, "http://example2.com", "Title2", ""),
        Tab(3, "http://example3.com", "Title3", "")
    };

    QTest::newRow("no_initial_tabs") << noInitialTabs << Tab(1, "http://example.com", "Title1", "") << 0 << 5 << true;
    QTest::newRow("add_new_tab") << threeTabs << Tab(4, "http://example4.com", "Title4", "") << 0 << 5 << true;
    QTest::newRow("activate_live_tab") << threeTabs << Tab(1, "http://example1.com", "Title1", "") << 0 << 5 << false;
    QTest::newRow("activate_dead_tab") << threeTabs << Tab(1, "http://example1.com", "Title1", "") << 0 << 2 << true;
    QTest::newRow("activate_active_tab") << threeTabs << Tab(3, "http://example3.com", "Title3", "") << 0 << 5 << false;
}

void tst_webpages::page()
{
    QFETCH(QList<Tab>, initialTabs);
    QFETCH(Tab, tab);
    QFETCH(int, parentId);
    QFETCH(int, maxLiveTabs);
    QFETCH(bool, isPageCreationExpected);

    // set up test case

    EXPECT_CALL(*QMozContext::GetInstance(), setPixelRatio(_));
    DeclarativeWebContainer webContainer;
    m_webPages->initialize(&webContainer);

    DeclarativeWebPage* page = nullptr;

    m_webPages->setMaxLivePages(maxLiveTabs);
    foreach (Tab initialTab, initialTabs) {
        page = new DeclarativeWebPage();
        EXPECT_CALL(*page, tabId()).Times(2).WillRepeatedly(Return(initialTab.tabId()));
        EXPECT_CALL(*page, uniqueID()).Times(AnyNumber()).WillRepeatedly(Return(initialTab.tabId()));
        EXPECT_CALL(*page, parentId()).Times(initialTab.tabId() == 1 && (tab.tabId() != 1 || maxLiveTabs < initialTabs.count()) ? 1 : 2);
        EXPECT_CALL(*page, resumeView()).Times(AnyNumber());
        EXPECT_CALL(*page, update()).Times(initialTab.tabId() == 1 && tab.tabId() == 1 && maxLiveTabs > initialTabs.count() ? 2 : 1);
        EXPECT_CALL(*page, completed()).WillOnce(Return(true));
        EXPECT_CALL(*page, contentRect()).Times(AnyNumber()).WillRepeatedly(Return(QRectF()));
        EXPECT_CALL(m_pageFactory, createWebPage(_, _, _)).WillOnce(Return(page));
        if (!(initialTab.tabId() == 3 && tab.tabId() == 3)) {
            // no need to suspend active view
            EXPECT_CALL(*page, suspendView());
            EXPECT_CALL(*page, loading()).WillOnce(Return(false));
        }

        m_webPages->page(initialTab);
    }


    // actual test
    if (isPageCreationExpected) {
        page = new DeclarativeWebPage();
        if (!(tab.tabId() == 1 && maxLiveTabs < initialTabs.count())) {
            EXPECT_CALL(*page, tabId()).WillOnce(Return(tab.tabId()));
        }
        EXPECT_CALL(*page, uniqueID()).WillOnce(Return(tab.tabId()));
        EXPECT_CALL(*page, parentId()).Times(initialTabs.count() ? 2 : 1).WillRepeatedly(Return(parentId));
        EXPECT_CALL(*page, resumeView());
        EXPECT_CALL(*page, update());
        EXPECT_CALL(*page, completed()).WillOnce(Return(true));
        EXPECT_CALL(*page, setResurrectedContentRect(_)).Times(tab.tabId() == 1 && maxLiveTabs < initialTabs.count() ? 1 : 0);
        EXPECT_CALL(m_pageFactory, createWebPage(_, _, _)).WillOnce(Return(page));
    }
    WebPageActivationData data = m_webPages->page(tab, parentId);
    if (tab.tabId() == 3) {
        // the third tab is active already
        QVERIFY(!data.activated);
    } else {
        QVERIFY(data.activated);
    }
    QVERIFY(data.webPage);
}

void tst_webpages::pageWithBrokenPageFactory()
{
    EXPECT_CALL(m_pageFactory, createWebPage(_, _, _)).WillOnce(Return(nullptr));
    WebPageActivationData data = m_webPages->page(Tab(1, "http://example.com", "Title1", ""), 0);
    QVERIFY(!data.activated);
    QVERIFY(!data.webPage);
}

void tst_webpages::release_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");
    QTest::addColumn<int>("tabId");
    QTest::addColumn<int>("maxLiveTabs");
    QTest::addColumn<int>("expectedCount");

    QList<Tab> noInitialTabs;
    QList<Tab> threeTabs {
        Tab(1, "http://example1.com", "Title1", ""),
        Tab(2, "http://example2.com", "Title2", ""),
        Tab(3, "http://example3.com", "Title3", "")
    };

    QTest::newRow("release_from_empty_list") << noInitialTabs << 1 << 5 << 0;
    QTest::newRow("release_non_existent_tab") << threeTabs << 10 << 5 << 3;
    QTest::newRow("release_live") << threeTabs << 2 << 5 << 2;
    QTest::newRow("release_virtualized") << threeTabs << 1 << 2 << 2;
}

void tst_webpages::release()
{
    QFETCH(QList<Tab>, initialTabs);
    QFETCH(int, tabId);
    QFETCH(int, maxLiveTabs);
    QFETCH(int, expectedCount);

    m_webPages->setMaxLivePages(maxLiveTabs);
    DeclarativeWebPage* page = nullptr;

    foreach (Tab initialTab, initialTabs) {
        page = new DeclarativeWebPage();
        EXPECT_CALL(*page, tabId()).Times(AnyNumber()).WillRepeatedly(Return(initialTab.tabId()));
        EXPECT_CALL(*page, uniqueID()).Times(AnyNumber()).WillRepeatedly(Return(initialTab.tabId()));
        EXPECT_CALL(*page, parentId()).Times(AnyNumber());
        EXPECT_CALL(*page, resumeView()).Times(AnyNumber());
        EXPECT_CALL(*page, update()).Times(AnyNumber());
        EXPECT_CALL(*page, completed()).WillOnce(Return(true));
        EXPECT_CALL(*page, contentRect()).Times(AnyNumber()).WillRepeatedly(Return(QRectF()));
        EXPECT_CALL(*page, loading()).Times(AnyNumber()).WillRepeatedly(Return(false));
        EXPECT_CALL(*page, suspendView()).Times(AnyNumber());

        EXPECT_CALL(m_pageFactory, createWebPage(_, _, _)).WillOnce(Return(page));
        m_webPages->page(initialTab);
    }

    m_webPages->release(tabId);
    QCOMPARE(m_webPages->count(), expectedCount);
}

void tst_webpages::clear_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");

    QList<Tab> noInitialTabs;
    QList<Tab> threeTabs {
        Tab(1, "http://example1.com", "Title1", ""),
        Tab(2, "http://example2.com", "Title2", ""),
        Tab(3, "http://example3.com", "Title3", "")
    };

    QTest::newRow("empty_queue") << noInitialTabs;
    QTest::newRow("non_empty_queue") << threeTabs;
}

void tst_webpages::clear()
{
    QFETCH(QList<Tab>, initialTabs);

    NiceMock<DeclarativeWebPage>* page = nullptr;

    foreach (Tab initialTab, initialTabs) {
        page = new NiceMock<DeclarativeWebPage>();
        EXPECT_CALL(*page, tabId()).Times(AnyNumber()).WillRepeatedly(Return(initialTab.tabId()));
        EXPECT_CALL(*page, uniqueID()).Times(AnyNumber()).WillRepeatedly(Return(initialTab.tabId()));
        EXPECT_CALL(*page, completed()).WillOnce(Return(true));
        EXPECT_CALL(*page, contentRect()).Times(AnyNumber()).WillRepeatedly(Return(QRectF()));

        EXPECT_CALL(m_pageFactory, createWebPage(_, _, _)).WillOnce(Return(page));
        m_webPages->page(initialTab);
    }

    m_webPages->clear();
    QCOMPARE(m_webPages->count(), 0);
}

void tst_webpages::parentTabId_data()
{
    QTest::addColumn<QList<Tab> >("initialTabs");
    QTest::addColumn<QList<int> >("initialParentIds");
    QTest::addColumn<int>("tabId");
    QTest::addColumn<int>("expectedParentId");

    QList<Tab> threeTabs {
        Tab(1, "http://example1.com", "Title1", ""),
        Tab(2, "http://example2.com", "Title2", ""),
        Tab(3, "http://example3.com", "Title3", "")
    };

    QList<int> threeParentIds {0, 1, 0};

    QTest::newRow("no_parent") << threeTabs << threeParentIds << 1 << 0;
    QTest::newRow("parent1") << threeTabs << threeParentIds << 2 << 1;
}

void tst_webpages::parentTabId()
{
    QFETCH(QList<Tab>, initialTabs);
    QFETCH(QList<int>, initialParentIds);
    QFETCH(int, tabId);
    QFETCH(int, expectedParentId);

    NiceMock<DeclarativeWebPage>* page = nullptr;

    for (int i = 0; i < initialTabs.count(); i++) {
        Tab initialTab = initialTabs.at(i);
        int parentId = initialParentIds.at(i);

        page = new NiceMock<DeclarativeWebPage>();
        EXPECT_CALL(*page, parentId()).Times(AnyNumber()).WillRepeatedly(Return(parentId));
        EXPECT_CALL(*page, tabId()).Times(AnyNumber()).WillRepeatedly(Return(initialTab.tabId()));
        EXPECT_CALL(*page, uniqueID()).Times(AnyNumber()).WillRepeatedly(Return(initialTab.tabId()));
        EXPECT_CALL(*page, completed()).WillOnce(Return(true));
        EXPECT_CALL(*page, contentRect()).Times(AnyNumber()).WillRepeatedly(Return(QRectF()));

        EXPECT_CALL(m_pageFactory, createWebPage(_, _, _)).WillOnce(Return(page));
        m_webPages->page(initialTab);
    }

    QCOMPARE(m_webPages->parentTabId(tabId), expectedParentId);
}

QTEST_MAIN(tst_webpages)
#include "tst_webpages.moc"
