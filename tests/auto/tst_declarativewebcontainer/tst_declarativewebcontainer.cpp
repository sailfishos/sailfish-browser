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
#include <webengine.h>

#include "declarativewebcontainer.h"
#include "declarativewebpage.h"
#include "privatetabmodel.h"
#include "settingmanager.h"
#include "tab.h"

#define NEXT_TAB_ID 1000

using ::testing::Return;
using ::testing::_;

class tst_declarativewebcontainer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void setWebPage();
    void setTabModel();
    void setForeground();
    void setMaxLiveTabCount_data();
    void setMaxLiveTabCount();
    void setPrivateMode();
    void loading();
    void setChromeWindow();
    void load();
    void reload();
    void goBackAndGoForward();
    void activatePage();
    void releasePage();

private:
    QPointer<DeclarativeWebContainer> m_webContainer;
};

void tst_declarativewebcontainer::initTestCase()
{
}

void tst_declarativewebcontainer::cleanupTestCase()
{
    delete SailfishOS::WebEngine::instance();
}

void tst_declarativewebcontainer::init()
{
    SettingManager::instance()->setAutostartPrivateBrowsing(false);
    m_webContainer = new DeclarativeWebContainer();
}

void tst_declarativewebcontainer::cleanup()
{
    delete m_webContainer;
}

void tst_declarativewebcontainer::setWebPage()
{
    // Set up
    DeclarativeWebPage page;
    QQuickView fakeChromeWindow;
    m_webContainer->m_chromeWindow = &fakeChromeWindow;

    QSignalSpy contentItemChangedSpy(m_webContainer, SIGNAL(contentItemChanged()));
    QSignalSpy tabIdChangedSpy(m_webContainer, SIGNAL(tabIdChanged()));
    QSignalSpy loadingChangedSpy(m_webContainer, SIGNAL(loadingChanged()));
    QSignalSpy focusObjectChangedSpy(m_webContainer, SIGNAL(focusObjectChanged(QObject*)));
    QSignalSpy canGoBackChangedSpy(m_webContainer, SIGNAL(canGoBackChanged()));
    QSignalSpy canGoForwardChangedSpy(m_webContainer, SIGNAL(canGoForwardChanged()));
    QSignalSpy urlChangedSpy(m_webContainer, SIGNAL(urlChanged()));
    QSignalSpy titleChangedSpy(m_webContainer, SIGNAL(titleChanged()));

    // Setting page
    EXPECT_CALL(page, setWindow(m_webContainer.data()));
    EXPECT_CALL(page, updateContentOrientation(_));
    EXPECT_CALL(page, loadProgress()).WillOnce(Return(0));
    m_webContainer->setWebPage(&page);
    QCOMPARE(m_webContainer->m_webPage.data(), &page);
    QCOMPARE(contentItemChangedSpy.count(), 1);
    QCOMPARE(tabIdChangedSpy.count(), 1);
    QCOMPARE(loadingChangedSpy.count(), 1);
    QCOMPARE(focusObjectChangedSpy.count(), 1);
    QList<QVariant> arguments = focusObjectChangedSpy.at(0);
    QCOMPARE(arguments.at(0).value<DeclarativeWebPage*>(), &page);
    QCOMPARE(canGoBackChangedSpy.count(), 1);
    QCOMPARE(canGoForwardChangedSpy.count(), 1);
    QCOMPARE(urlChangedSpy.count(), 1);
    QCOMPARE(titleChangedSpy.count(), 1);

    // Unsetting page
    m_webContainer->setWebPage(0);
    QCOMPARE(!!m_webContainer->m_webPage.data(), false);
    QCOMPARE(contentItemChangedSpy.count(), 2);
    QCOMPARE(tabIdChangedSpy.count(), 2);
    QCOMPARE(loadingChangedSpy.count(), 2);
    QCOMPARE(focusObjectChangedSpy.count(), 2);
    QCOMPARE(arguments.at(0).toInt(), 0);
    QCOMPARE(canGoBackChangedSpy.count(), 2);
    QCOMPARE(canGoForwardChangedSpy.count(), 2);
    QCOMPARE(urlChangedSpy.count(), 2);
    QCOMPARE(titleChangedSpy.count(), 2);
}

void tst_declarativewebcontainer::setTabModel()
{
    // Set up
    PrivateTabModel model(NEXT_TAB_ID);
    model.setWaitingForNewTab(false);
    Tab tab;
    model.m_tabs.append(tab);

    QSignalSpy countChangeSpy(&model, SIGNAL(countChanged()));

    bool wasInitialized = m_webContainer->m_initialized;
    m_webContainer->m_initialized = true;
    // Setting model1
    m_webContainer->setTabModel(&model);
    QCOMPARE(m_webContainer->m_model.data(), &model);
    QCOMPARE(model.waitingForNewTab(), true);
    QCOMPARE(countChangeSpy.count(), 1);

    // Unsetting model
    m_webContainer->setTabModel(0);
    QCOMPARE(!!m_webContainer->m_model.data(), false);
    QCOMPARE(model.waitingForNewTab(), false);
    m_webContainer->m_initialized = wasInitialized;
}

void tst_declarativewebcontainer::setForeground()
{
    // Set up
    m_webContainer->m_foreground = false;

    QSignalSpy foregroundChangedSpy(m_webContainer, SIGNAL(foregroundChanged()));

    // setting true
    m_webContainer->setForeground(true);
    QCOMPARE(m_webContainer->foreground(), true);
    QCOMPARE(foregroundChangedSpy.count(), 1);

    // setting false
    m_webContainer->setForeground(false);
    QCOMPARE(m_webContainer->foreground(), false);
    QCOMPARE(foregroundChangedSpy.count(), 2);
}

void tst_declarativewebcontainer::setMaxLiveTabCount_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("valid_count") << 3 << true;
    QTest::newRow("invalid_count_1") << 0 << false;
    QTest::newRow("invalid_count_2") << -100 << false;
}

void tst_declarativewebcontainer::setMaxLiveTabCount()
{
    QFETCH(int, count);
    QFETCH(bool, isValid);

    QSignalSpy maxLiveTabCountChangedSpy(m_webContainer, SIGNAL(maxLiveTabCountChanged()));

    m_webContainer->setMaxLiveTabCount(count);

    if (isValid) {
        QCOMPARE(maxLiveTabCountChangedSpy.count(), 1);
    } else {
        QCOMPARE(maxLiveTabCountChangedSpy.count(), 0);
    }
}

void tst_declarativewebcontainer::setPrivateMode()
{
    QSignalSpy privateModeChangedSpy(m_webContainer, SIGNAL(privateModeChanged()));
    QSignalSpy contentItemChangedSpy(m_webContainer, SIGNAL(contentItemChanged()));

    PrivateTabModel model(NEXT_TAB_ID);
    Tab tab;
    model.m_tabs.append(tab);
    m_webContainer->m_privateTabModel = &model;

    m_webContainer->setPrivateMode(true);
    QCOMPARE(m_webContainer->privateMode(), true);
    QCOMPARE(m_webContainer->m_settingManager->autostartPrivateBrowsing(), true);
    QCOMPARE(privateModeChangedSpy.count(), 1);
    QCOMPARE(contentItemChangedSpy.count(), 0);

    // the same value => no update
    m_webContainer->setPrivateMode(true);
    QCOMPARE(m_webContainer->privateMode(), true);
    QCOMPARE(m_webContainer->m_settingManager->autostartPrivateBrowsing(), true);
    QCOMPARE(privateModeChangedSpy.count(), 1);

    QSignalSpy tabIdChangedSpy(m_webContainer, SIGNAL(tabIdChanged()));
    m_webContainer->setPrivateMode(false);
    QCOMPARE(m_webContainer->privateMode(), false);
    QCOMPARE(m_webContainer->m_settingManager->autostartPrivateBrowsing(), false);
    QCOMPARE(privateModeChangedSpy.count(), 2);
    QCOMPARE(tabIdChangedSpy.count(), 1);
    QCOMPARE(contentItemChangedSpy.count(), 1);
}

void tst_declarativewebcontainer::loading()
{
    DeclarativeWebPage page;
    PrivateTabModel model(NEXT_TAB_ID);
    Tab tab;
    model.m_tabs.append(tab);

    // Empty container => can't be loading
    QCOMPARE(m_webContainer->loading(), false);

    // Page is not loading => false
    EXPECT_CALL(page, loadProgress());
    m_webContainer->setWebPage(&page);
    EXPECT_CALL(page, loading()).WillOnce(Return(false));
    QCOMPARE(m_webContainer->loading(), false);

    // No page, but non-empty model => true
    m_webContainer->setWebPage(0);
    m_webContainer->setTabModel(&model);
    QCOMPARE(m_webContainer->loading(), true);
}

void tst_declarativewebcontainer::setChromeWindow()
{
    QQuickView view;

    QSignalSpy chromeWindowChangedSpy(m_webContainer, SIGNAL(chromeWindowChanged()));

    m_webContainer->setChromeWindow(&view);

    QCOMPARE(chromeWindowChangedSpy.count(), 1);
    QCOMPARE(m_webContainer->chromeWindow(), &view);
}

void tst_declarativewebcontainer::load()
{
    // No page and no model set => set initial url
    EXPECT_CALL(*SailfishOS::WebEngine::instance(), initialized()).WillOnce(Return(false));
    m_webContainer->load(QString(), QString(), true);
    QCOMPARE(m_webContainer->m_initialUrl, QString("about:blank"));

    // Test the path for uninitialized container => set initial url
    DeclarativeWebPage page;
    EXPECT_CALL(page, loadProgress());
    m_webContainer->setWebPage(&page);
    EXPECT_CALL(page, completed()).WillOnce(Return(false));
    EXPECT_CALL(*SailfishOS::WebEngine::instance(), initialized()).WillOnce(Return(false));
    m_webContainer->load(QString("http://example1.com"), QString(), true);
    QCOMPARE(m_webContainer->m_initialUrl, QString("http://example1.com"));

    // Initialized container, empty model => add new tab to model
    PrivateTabModel model(NEXT_TAB_ID);
    m_webContainer->setTabModel(&model);
    EXPECT_CALL(*SailfishOS::WebEngine::instance(), initialized()).WillOnce(Return(true));
    EXPECT_CALL(page, completed()).WillOnce(Return(false));
    m_webContainer->load(QString(), QString(), true);

    // There is a complete web page => page->loadTab()
    EXPECT_CALL(page, completed()).WillOnce(Return(true));
    QString testurl("http://example2.com");
    EXPECT_CALL(page, loadTab(testurl, true));
    m_webContainer->load(testurl, QString(), true);
}

void tst_declarativewebcontainer::reload()
{
    // Set up
    PrivateTabModel model(NEXT_TAB_ID);
    model.addTab(QString("http://example.com"), QString("Test title"), 0);
    m_webContainer->setTabModel(&model);

    m_webContainer->reload(false);

    // TODO: figure out how to test reloading active page
}

void tst_declarativewebcontainer::goBackAndGoForward()
{
    m_webContainer->goBack();
    m_webContainer->goForward();
    // TODO: figure out how to test cases when `canGoBack() == true`.
}

void tst_declarativewebcontainer::activatePage()
{
    PrivateTabModel model(NEXT_TAB_ID);
    Tab tab;
    tab.setTabId(1);

    bool res = m_webContainer->activatePage(tab, true, 0);
    QCOMPARE(res, false);
}

void tst_declarativewebcontainer::releasePage()
{
    EXPECT_CALL(*SailfishOS::WebEngine::instance(), PostCompositorTask(_, m_webContainer.data()));
    m_webContainer->releasePage(1);
}

QTEST_MAIN(tst_declarativewebcontainer)
#include "tst_declarativewebcontainer.moc"
