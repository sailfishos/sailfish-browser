/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativewebcontainer.h"
#include "declarativetabmodel.h"
#include "declarativewebpage.h"
#include "dbmanager.h"
#include "downloadmanager.h"
#include "declarativewebutils.h"

#include <QPointer>
#include <QTimerEvent>
#include <QQuickWindow>
#include <QDir>
#include <QTransform>
#include <QStandardPaths>
#include <QtConcurrentRun>
#include <QGuiApplication>
#include <QScreen>
#include <QMetaMethod>

#include <qmozcontext.h>
#include <QGuiApplication>

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

DeclarativeWebContainer::DeclarativeWebContainer(QQuickItem *parent)
    : QQuickItem(parent)
    , m_webPage(0)
    , m_model(0)
    , m_webPageComponent(0)
    , m_settingManager(SettingManager::instance())
    , m_foreground(true)
    , m_allowHiding(true)
    , m_popupActive(false)
    , m_portrait(true)
    , m_fullScreenMode(false)
    , m_fullScreenHeight(0.0)
    , m_inputPanelVisible(false)
    , m_inputPanelHeight(0.0)
    , m_inputPanelOpenHeight(0.0)
    , m_toolbarHeight(0.0)
    , m_tabId(0)
    , m_loading(false)
    , m_loadProgress(0)
    , m_canGoForward(false)
    , m_canGoBack(false)
    , m_realNavigation(false)
    , m_readyToLoad(false)
    , m_deferredReload(false)
{
    m_webPages.reset(new WebPages(this));
    setFlag(QQuickItem::ItemHasContents, true);
    connect(DownloadManager::instance(), SIGNAL(downloadStarted()), this, SLOT(onDownloadStarted()));
    connect(this, SIGNAL(_readyToLoadChanged()), this, SLOT(onReadyToLoad()));
    connect(this, SIGNAL(portraitChanged()), this, SLOT(resetHeight()));
    connect(this, SIGNAL(enabledChanged()), this, SLOT(handleEnabledChanged()));

    QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheLocation);
    if(!dir.exists() && !dir.mkpath(cacheLocation)) {
        qWarning() << "Can't create directory "+ cacheLocation;
        return;
    }

    connect(this, SIGNAL(heightChanged()), this, SLOT(sendVkbOpenCompositionMetrics()));
    connect(this, SIGNAL(widthChanged()), this, SLOT(sendVkbOpenCompositionMetrics()));

    qApp->installEventFilter(this);
}

DeclarativeWebContainer::~DeclarativeWebContainer()
{
    // Disconnect all signal slot connections
    if (m_webPage) {
        disconnect(m_webPage, 0, 0, 0);
    }
}

DeclarativeWebPage *DeclarativeWebContainer::webPage() const
{
    return m_webPage;
}

void DeclarativeWebContainer::setWebPage(DeclarativeWebPage *webPage)
{
    if (m_webPage != webPage) {
        m_webPage = webPage;

        if (m_webPage) {
            m_tabId = m_webPage->tabId();
        } else {
            m_tabId = 0;
        }

        emit contentItemChanged();
        emit tabIdChanged();

        updateUrl(url());
        updateTitle(title());
    }
}

DeclarativeTabModel *DeclarativeWebContainer::tabModel() const
{
    return m_model;
}

void DeclarativeWebContainer::setTabModel(DeclarativeTabModel *model)
{
    if (m_model != model) {
        if (m_model) {
            disconnect(m_model);
        }

        m_model = model;
        if (m_model) {
            connect(m_model, SIGNAL(activeTabChanged(int,int,bool)), this, SLOT(onActiveTabChanged(int,int,bool)));
            connect(m_model, SIGNAL(loadedChanged()), this, SLOT(onModelLoaded()));
            connect(m_model, SIGNAL(tabClosed(int)), this, SLOT(releasePage(int)));
            connect(m_model, SIGNAL(tabsCleared()), this, SLOT(onTabsCleared()));
            connect(m_model, SIGNAL(newTabRequested(QString,QString)), this, SLOT(onNewTabRequested(QString,QString)));
        }
        emit tabModelChanged();
    }
}

bool DeclarativeWebContainer::foreground() const
{
    return m_foreground;
}

void DeclarativeWebContainer::setForeground(bool active)
{
    if (m_foreground != active) {
        m_foreground = active;

        if (!m_foreground) {
            // Respect content height when browser brought back from home
            resetHeight(true);
        }
        emit foregroundChanged();
    }
}

int DeclarativeWebContainer::maxLiveTabCount() const
{
    return m_webPages->maxLivePages();
}

void DeclarativeWebContainer::setMaxLiveTabCount(int count)
{
    if (m_webPages->setMaxLivePages(count)) {
        emit maxLiveTabCountChanged();
    }
}

bool DeclarativeWebContainer::background() const
{
    return m_webPage ? m_webPage->background() : false;
}

bool DeclarativeWebContainer::loading() const
{
    return m_webPage ? m_webPage->loading() : m_model->count();
}

int DeclarativeWebContainer::loadProgress() const
{
    return m_loadProgress;
}

void DeclarativeWebContainer::setLoadProgress(int loadProgress)
{
    if (m_loadProgress != loadProgress) {
        m_loadProgress = loadProgress;
        emit loadProgressChanged();
    }
}

bool DeclarativeWebContainer::inputPanelVisible() const
{
    return m_inputPanelVisible;
}

qreal DeclarativeWebContainer::inputPanelHeight() const
{
    return m_inputPanelHeight;
}

void DeclarativeWebContainer::setInputPanelHeight(qreal height)
{
    if (m_inputPanelHeight != height) {
        bool imVisibleChanged = false;
        m_inputPanelHeight = height;
        if (isEnabled()) {
            if (m_inputPanelHeight == 0) {
                if (m_inputPanelVisible) {
                    m_inputPanelVisible = false;
                    imVisibleChanged = true;
                }
            } else if (m_inputPanelHeight == m_inputPanelOpenHeight) {
                if (!m_inputPanelVisible) {
                    m_inputPanelVisible = true;
                    imVisibleChanged = true;
                }
            }
        }

        if (imVisibleChanged) {
            emit inputPanelVisibleChanged();
        }

        emit inputPanelHeightChanged();
    }
}

bool DeclarativeWebContainer::canGoForward() const
{
    return m_canGoForward;
}

bool DeclarativeWebContainer::canGoBack() const
{
    return m_canGoBack;
}

int DeclarativeWebContainer::tabId() const
{
    return m_tabId;
}

QString DeclarativeWebContainer::title() const
{
    return m_webPage ? m_webPage->title() : m_title;
}

QString DeclarativeWebContainer::url() const
{
    return m_webPage ? m_webPage->url().toString() : m_url;
}

bool DeclarativeWebContainer::readyToLoad() const
{
    return m_readyToLoad;
}

void DeclarativeWebContainer::setReadyToLoad(bool readyToLoad)
{
    static bool browserReady = false;
    if (!browserReady && readyToLoad) {
        bool wasClearHistoryRequested = m_settingManager->clearHistoryRequested();
        browserReady = true;
        m_settingManager->initialize();
        if (m_model && wasClearHistoryRequested) {
            readyToLoad = false;
            emit m_model->countChanged();
        }
    }

    if (m_readyToLoad != readyToLoad) {
        m_readyToLoad = readyToLoad;
        emit _readyToLoadChanged();
    }
}

bool DeclarativeWebContainer::isActiveTab(int tabId)
{
    return m_webPage && m_webPage->tabId() == tabId;
}

void DeclarativeWebContainer::goForward()
{
    if (m_canGoForward && m_webPage) {
        m_canGoForward = false;
        emit canGoForwardChanged();

        if (!m_canGoBack) {
            m_canGoBack = true;
            emit canGoBackChanged();
        }

        m_webPage->setBackForwardNavigation(true);
        m_realNavigation = m_webPage->canGoForward();
        DBManager::instance()->goForward(m_webPage->tabId());
        if (m_realNavigation) {
            m_webPage->goForward();
        }
    }
}

void DeclarativeWebContainer::goBack()
{
    if (m_canGoBack && m_webPage) {
        m_canGoBack = false;
        emit canGoBackChanged();

        if (!m_canGoForward) {
            m_canGoForward = true;
            emit canGoForwardChanged();
        }

        m_webPage->setBackForwardNavigation(true);
        m_realNavigation = m_webPage->canGoBack();
        // When executing non real back navigation, we're adding
        // an entry to forward history of the engine. For instance, in sequence like
        // link X, link Y, restart browser, and navigate back. This sequence triggers
        // an url loading when going back. This means that there is a clean
        // back history in engine. If you now navigate forward, we load the url and not
        // use engine's forward.
        // After this back/forward starts working correctly. As loading indicator can
        // be visible also when using engine navigation it makes this error to hard to spot.
        // In addition, as history is based on browser side database, user cannot navigate
        // beyond back history from user interface. However, JavaScript functions maybe do harm.
        // TODO: We should resurrect the back forward history for page instance
        // when a new tab is created.
        DBManager::instance()->goBack(m_webPage->tabId());
        if (m_realNavigation) {
            m_webPage->goBack();
        }
    }
}

bool DeclarativeWebContainer::activatePage(int tabId, bool force)
{
    if (!m_model) {
        return false;
    }

    m_webPages->initialize(this, m_webPageComponent.data());
    if ((m_model->loaded() || force) && tabId > 0 && m_webPages->initialized()) {
        WebPageActivationData activationData = m_webPages->page(tabId, m_model->newTabParentId());
        activationData.webPage->disconnect(this);
        setWebPage(activationData.webPage);
        // Reset always height so that orentation change is taken into account.
        m_webPage->forceChrome(false);
        m_webPage->setChrome(true);
        setLoadProgress(m_webPage->loadProgress());

        connect(m_webPage, SIGNAL(imeNotification(int,bool,int,int,QString)),
                this, SLOT(imeNotificationChanged(int,bool,int,int,QString)), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(windowCloseRequested()), this, SLOT(closeWindow()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(urlChanged()), this, SLOT(onPageUrlChanged()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(loadingChanged()), this, SIGNAL(loadingChanged()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(titleChanged()), this, SLOT(onPageTitleChanged()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(domContentLoadedChanged()), this, SLOT(sendVkbOpenCompositionMetrics()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(backgroundChanged()), this, SIGNAL(backgroundChanged()), Qt::UniqueConnection);
        return activationData.activated;
    }
    return false;
}

void DeclarativeWebContainer::loadNewTab(QString url, QString title, int parentId)
{
    m_model->newTabData(url, title, webPage(), parentId);
    // This could handle new page creation directly if/
    // when connection helper is accessible from QML.
    emit triggerLoad(url, title);
}

bool DeclarativeWebContainer::alive(int tabId)
{
    return m_webPages->alive(tabId);
}

void DeclarativeWebContainer::dumpPages() const
{
    m_webPages->dumpPages();
}

bool DeclarativeWebContainer::eventFilter(QObject *obj, QEvent *event)
{
    // Hiding stops rendering. Don't pass it through if hiding is not allowed.
    if (event->type() == QEvent::Expose && window() && !window()->isExposed() && !m_allowHiding) {
        return true;
    }
    return QObject::eventFilter(obj, event);
}

void DeclarativeWebContainer::resetHeight(bool respectContentHeight)
{
    if (!m_webPage) {
        return;
    }

    m_webPage->resetHeight(respectContentHeight);
}

void DeclarativeWebContainer::imeNotificationChanged(int state, bool open, int cause, int focusChange, const QString &type)
{
    Q_UNUSED(open)
    Q_UNUSED(cause)
    Q_UNUSED(focusChange)
    Q_UNUSED(type)

    // QmlMozView's input context open is actually intention (0 closed, 1 opened).
    // cause 3 equals InputContextAction::CAUSE_MOUSE nsIWidget.h
    if (state == 1 && cause == 3) {
        // For safety reset height based on contentHeight before going to "boundHeightControl" state
        // so that when vkb is closed we get correctly reset height back.
        resetHeight(true);
        if (!m_inputPanelVisible) {
            m_inputPanelVisible = true;
            emit inputPanelVisibleChanged();
        }
    }
}

qreal DeclarativeWebContainer::contentHeight() const
{
    if (m_webPage) {
        return m_webPage->contentHeight();
    } else {
        return 0.0;
    }
}

void DeclarativeWebContainer::handleEnabledChanged()
{
    // If dialog has been opened, we need to verify that input panel is not visible.
    // This might happen when the user fills in login details to a form and
    // presses enter to accept the form after which PasswordManagerDialog is pushed to pagestack
    // on top the BrowserPage. Once PassowordManagerDialog is accepted/rejected
    // this condition can be met. If active changes to true before keyboard is fully closed,
    // then the inputPanelVisibleChanged() signal is emitted by setInputPanelHeight.
    if (isEnabled() && m_inputPanelHeight == 0 && m_inputPanelVisible) {
        m_inputPanelVisible = false;
        emit inputPanelVisibleChanged();
    }
}

void DeclarativeWebContainer::onActiveTabChanged(int oldTabId, int activeTabId, bool loadActiveTab)
{
    if (activeTabId <= 0) {
        return;
    }
    const Tab &tab = m_model->activeTab();
#if DEBUG_LOGS
    qDebug() << "canGoBack = " << m_canGoBack << "canGoForward = " << m_canGoForward << &tab;
#endif

    updateNavigationStatus(tab);
    updateUrl(tab.url());

    /**
     * TODO: We should update title here as well.
     * Reason not to do so yet is the way how database writes are synchronized to model and
     * webcontainer states. Currently DeclarativeTabModel do not ignore callbacks from database
     * worker if another operation made call obsolete.
     *
     * For example:
     * DeclarativeWebPage::loadTab loads an url when the url changes onPageUrlChanged
     * slot gets called and update url of webcontainer and initiates async write to database without title.
     * After a while db replies that tab has changed including data.
     * Right after onPageUrlChanged slot is called, onPageTitleChanged slot is called. The onPageTitleChanged
     * triggers async write to database with title.
     * Due to async nature, order of operations might look like (loadTab):
     *   1) onPageUrlChanged
     *   2) onPageTitleChanged (so far all would be good)
     *   3) DB call from to DeclarativeTabModel::tabChanged (with url, callback from 1)
     *   4) DB call from to DeclarativeTabModel::tabChanged (with url & title, callback from 2)
     *
     * Callbacks should be bound to triggered operation allowing other operations
     * to ignore callback that are not needed anymore. E.g. in above case
     * 2th step should mark for model that callback coming from onPageUrlChanged 3rd can be
     * ignored as we know at that moment there is another callback soon coming.
     */
    // updateTitle(tab.title());
    // Reverts title change handling that was introduced in commit ad85e79d.

    if (m_tabId != activeTabId) {
        m_tabId = activeTabId;
        emit tabIdChanged();
    }

    if (m_model->hasNewTabData() || !loadActiveTab) {
        return;
    }

    // Switch to different tab.
    if (oldTabId != activeTabId) {
        QString tabUrl = tab.url();

        if (activatePage(activeTabId, true) && m_readyToLoad
                && (m_webPage->tabId() != activeTabId || m_webPage->url().toString() != tabUrl)) {
            emit triggerLoad(tabUrl, tab.title());
        }
    } else if (!m_realNavigation && isActiveTab(activeTabId) && m_webPage->backForwardNavigation()) {
        emit triggerLoad(tab.url(), tab.title());
    }
}

void DeclarativeWebContainer::onModelLoaded()
{
    // This signal handler is responsible for activating
    // the first page.
    bool firstUseDone = DeclarativeWebUtils::instance()->firstUseDone();
    if (m_model->hasNewTabData() || (m_model->count() == 0 && firstUseDone)) {
        activatePage(m_model->nextTabId(), true);
    } else if (m_model->count() > 0) {
        const Tab &tab = m_model->activeTab();
        activatePage(tab.tabId(), true);
    }
}

void DeclarativeWebContainer::onDownloadStarted()
{
    // This is not 100% solid. A new tab is created for every incoming
    // url. In slow network connectivity one can close previous tab or
    // create a new tab before downloadStarted is emitted
    // by DownloadManager. To get this to the 100%, we would need to
    // pass windowId of the active window when download is started and close
    // the passed windowId instead.
    if (m_model && m_model->hasNewTabData() && m_model->count() > 0 && m_webPage) {
        DeclarativeWebPage *previousWebPage = qobject_cast<DeclarativeWebPage *>(m_model->newTabPreviousPage());
        releasePage(m_webPage->tabId());
        if (previousWebPage) {
            activatePage(previousWebPage->tabId());
        } else if (m_model->count() == 0) {
            // Download doesn't add tab to model. Mimic
            // model change in case tabs count goes to zero.
            emit m_model->countChanged();
        }
    }
}

void DeclarativeWebContainer::onNewTabRequested(QString url, QString title)
{
    // An empty tab for cleaning previous navigation status.
    Tab tab;
    updateNavigationStatus(tab);

    if (m_webPage) {
        m_webPage->setVisible(false);
        m_webPage->setOpacity(1.0);
        setWebPage(0);
    }
    loadNewTab(url, title, 0);
}

void DeclarativeWebContainer::onReadyToLoad()
{
    // Triggered when tabs of tab model are available and QmlMozView is ready to load.
    // Load test
    // 1) tabModel.hasNewTabData -> loadTab (already activated view)
    // 2) browser starting and initial url given
    // 3) model has tabs, load active tab -> load (activate view when needed)
    // 4) load home page -> load (activate view when needed)

    // visible could be possible delay property for _readyToLoad if so wanted.
    if (!m_readyToLoad || !m_model) {
        return;
    }

    if (m_model->hasNewTabData()) {
        m_webPage->loadTab(m_model->newTabUrl(), false);
    } else if (!m_initialUrl.isEmpty()) {
        emit triggerLoad(m_initialUrl, "");
    } else if (m_model->count() > 0) {
        // Previous active tab is actived when tabs are loaded to the tabs tabModel.
        m_model->resetNewTabData();
        const Tab &tab = m_model->activeTab();
        emit triggerLoad(tab.url(), tab.title());
    } else {
        // This can happen only during startup.
        emit triggerLoad(DeclarativeWebUtils::instance()->homePage(), "");
    }
}

void DeclarativeWebContainer::onTabsCleared()
{
    m_webPages->clear();
    m_title = "";
    m_url = "";
    // TODO: Do this really emit signals? WebPage is behind a QPointer. Thus, it should be already null.
    // webPage destroyed signal could be used.
    setWebPage(0);
//    emit contentItemChanged();
//    emit titleChanged();
//    emit urlChanged();

    if (m_tabId != 0) {
        m_tabId = 0;
        emit tabIdChanged();
    }
}

int DeclarativeWebContainer::parentTabId(int tabId) const
{
    if (m_webPages) {
        return m_webPages->parentTabId(tabId);
    }
    return 0;
}

void DeclarativeWebContainer::releasePage(int tabId, bool virtualize)
{
    if (m_webPages) {
        m_webPages->release(tabId, virtualize);
        // Successfully destroyed. Emit relevant property changes.
        if (!m_webPage) {
            m_tabId = 0;
            if (m_canGoBack) {
                m_canGoBack = false;
                emit canGoBackChanged();
            }

            if (m_canGoForward) {
                m_canGoForward = false;
                emit canGoForwardChanged();
            }

            emit contentItemChanged();
            updateUrl("");
            updateTitle("");
            emit tabIdChanged();
        }
        m_model->resetNewTabData();
    }
}

void DeclarativeWebContainer::closeWindow()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage && m_model) {
        int parentPageTabId = parentTabId(webPage->tabId());
        // Closing only allowed if window was created by script
        if (parentPageTabId > 0) {
            m_model->activateTabById(parentPageTabId);
            m_model->removeTabById(webPage->tabId(), isActiveTab(webPage->tabId()));
        }
    }
}

void DeclarativeWebContainer::onPageUrlChanged()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage && m_model) {
        QString url = webPage->url().toString();
        if (url != "about:blank") {
            int tabId = webPage->tabId();
            bool activeTab = isActiveTab(tabId);
            // Update initial back / forward navigation state
            if (activeTab && !webPage->urlHasChanged()) {
                const Tab &tab = m_model->activeTab();
                updateNavigationStatus(tab);
            }

            // Initial url should be considered as navigation request that increases navigation history.
            bool initialLoad = m_initialUrl.isEmpty() && !webPage->urlHasChanged();
            m_model->updateUrl(tabId, activeTab, url, webPage->backForwardNavigation(), initialLoad);
            m_initialUrl = "";

            webPage->setUrlHasChanged(true);
            webPage->setBackForwardNavigation(false);
            if (activeTab && webPage == m_webPage) {
                updateUrl(url);
            }
        }
    }
}

void DeclarativeWebContainer::onPageTitleChanged()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage && m_model) {
        QString title = webPage->title();
        int tabId = webPage->tabId();
        bool activeTab = isActiveTab(tabId);
        m_model->updateTitle(tabId, activeTab, title);

        if (activeTab && webPage == m_webPage) {
            updateTitle(title);
        }
    }
}

void DeclarativeWebContainer::updateNavigationStatus(const Tab &tab)
{
    if (m_canGoForward != (tab.nextLink() > 0)) {
        m_canGoForward = tab.nextLink() > 0;
        emit canGoForwardChanged();
    }

    if (m_canGoBack != (tab.previousLink() > 0)) {
        m_canGoBack = tab.previousLink() > 0;
        emit canGoBackChanged();
    }
}

void DeclarativeWebContainer::updateVkbHeight()
{
    qreal vkbHeight = 0;
    // Keyboard rect is updated too late, when vkb hidden we cannot yet get size.
    // We need to send correct information to embedlite-components before virtual keyboard is open
    // so that when input element is focused contect is zoomed to the correct target (available area).
#if 0
    if (qGuiApp->inputMethod()) {
        vkbHeight = qGuiApp->inputMethod()->keyboardRectangle().height();
    }
#else
    // TODO: remove once keyboard height is not zero when hidden and take above #if 0 block into use.
    vkbHeight = 440;
    if (width() > height()) {
        vkbHeight = 340;
    }
#endif
    m_inputPanelOpenHeight = vkbHeight;
}

/**
 * Pass the newUrl. This should be used everywhere when
 * url is about to change. E.g. when updating current contentItem.
 * When both url and title are about to change call updateUrl first
 * as it is more natural that way.
 * @brief DeclarativeWebContainer::updateUrl
 * @param newUrl
 *
 * See also updateTitle
 */
void DeclarativeWebContainer::updateUrl(const QString &newUrl)
{
    if (m_url != newUrl) {
        m_url = newUrl;
        emit urlChanged();
    }
}

/**
 * Pass the newTitle. This should be used everywhere when
 * title is about to change. E.g. when updating current contentItem.
 * When both url and title are about to change call updateUrl first
 * as it is more natural that way.
 * @brief DeclarativeWebContainer::updateTitle
 * @param newTitle
 *
 * See also updateUrl
 */
void DeclarativeWebContainer::updateTitle(const QString &newTitle)
{
    if (m_title != newTitle) {
        m_title = newTitle;
        emit titleChanged();
    }
}

void DeclarativeWebContainer::sendVkbOpenCompositionMetrics()
{
    updateVkbHeight();

    QVariantMap map;

    // Round values to even numbers.
    int vkbOpenCompositionHeight = height() - m_inputPanelOpenHeight;
    int vkbOpenMaxCssCompositionWidth = width() / QMozContext::GetInstance()->pixelRatio();
    int vkbOpenMaxCssCompositionHeight = vkbOpenCompositionHeight / QMozContext::GetInstance()->pixelRatio();

    map.insert("compositionHeight", vkbOpenCompositionHeight);
    map.insert("maxCssCompositionWidth", vkbOpenMaxCssCompositionWidth);
    map.insert("maxCssCompositionHeight", vkbOpenMaxCssCompositionHeight);

    QVariant data(map);

    if (m_webPage) {
        m_webPage->sendAsyncMessage("embedui:vkbOpenCompositionMetrics", data);
    }
}
