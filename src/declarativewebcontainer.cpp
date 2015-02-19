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
#include "persistenttabmodel.h"
#include "privatetabmodel.h"
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
    , m_completed(false)
    , m_initialized(false)
    , m_privateMode(m_settingManager->autostartPrivateBrowsing())
{
    setFlag(QQuickItem::ItemHasContents, true);

    m_webPages = new WebPages(this);
    m_persistentTabModel = new PersistentTabModel(this);
    m_privateTabModel = new PrivateTabModel(this);

    setTabModel(privateMode() ? m_privateTabModel.data() : m_persistentTabModel.data());

    connect(DownloadManager::instance(), SIGNAL(initializedChanged()), this, SLOT(initialize()));
    connect(DownloadManager::instance(), SIGNAL(downloadStarted()), this, SLOT(onDownloadStarted()));
    connect(QMozContext::GetInstance(), SIGNAL(onInitialized()), this, SLOT(initialize()));
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
    connect(this, SIGNAL(foregroundChanged()), this, SLOT(updateWindowFlags()));

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
        // Disconnect previous page.
        if (m_webPage) {
            m_webPage->disconnect(this);
        }

        m_webPage = webPage;

        if (m_webPage) {
            m_tabId = m_webPage->tabId();
        } else {
            m_tabId = 0;
        }

        emit contentItemChanged();
        emit tabIdChanged();
        emit loadingChanged();

        updateUrl(url());
        updateTitle(title());

        setLoadProgress(m_webPage ? m_webPage->loadProgress() : 0);
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
            disconnect(m_model, 0, 0, 0);
        }

        m_model = model;
        if (m_model) {
            connect(m_model, SIGNAL(activeTabChanged(int,int,bool)), this, SLOT(onActiveTabChanged(int,int,bool)));
            connect(m_model, SIGNAL(loadedChanged()), this, SLOT(initialize()));
            connect(m_model, SIGNAL(tabClosed(int)), this, SLOT(releasePage(int)));
            connect(m_model, SIGNAL(tabsCleared()), this, SLOT(onTabsCleared()));
            connect(m_model, SIGNAL(newTabRequested(QString,QString,int)), this, SLOT(onNewTabRequested(QString,QString,int)));
        }
        emit tabModelChanged();
    }
}

bool DeclarativeWebContainer::completed() const
{
    return m_completed;
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

bool DeclarativeWebContainer::privateMode() const
{
    return m_privateMode;
}

void DeclarativeWebContainer::setPrivateMode(bool privateMode)
{
    if (m_privateMode != privateMode) {
        m_privateMode = privateMode;
        m_settingManager->setAutostartPrivateBrowsing(privateMode);
        updateMode();
        emit privateModeChanged();
    }
}

bool DeclarativeWebContainer::background() const
{
    return m_webPage ? m_webPage->background() : false;
}

bool DeclarativeWebContainer::loading() const
{
    if (m_webPage) {
        return m_webPage->loading();
    } else {
        return m_model ? m_model->count() : false;
    }
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

bool DeclarativeWebContainer::isActiveTab(int tabId)
{
    return m_webPage && m_webPage->tabId() == tabId;
}

void DeclarativeWebContainer::load(QString url, QString title, bool force)
{
    if (url.isEmpty()) {
        url = "about:blank";
    }

    if (m_webPage && m_webPage->viewReady()) {
        m_webPage->loadTab(url, force);
    } else if (!canInitialize()) {
        updateUrl(url);
        updateTitle(title);
    } else if (m_model && m_model->count() == 0) {
        // Browser running all tabs are closed.
        m_model->newTab(url, title);
    }
}

/**
 * @brief DeclarativeWebContainer::reload
 * Reloads the active tab. If not tabs exist this does nothing. If the page was
 * virtualized it will be resurrected.
 */
void DeclarativeWebContainer::reload(bool force)
{
    if (m_tabId > 0) {
        if (force && m_webPage && m_webPage->viewReady() && m_webPage->tabId() == m_tabId) {
            // Reload live active tab directly.
            m_webPage->reload();
        } else {
            loadTab(m_tabId, m_url, m_title, force);
        }
    }
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

bool DeclarativeWebContainer::activatePage(int tabId, bool force, int parentId)
{
    if (!m_model) {
        return false;
    }

    m_webPages->initialize(this, m_webPageComponent.data());
    if ((m_model->loaded() || force) && tabId > 0 && m_webPages->initialized()) {
        WebPageActivationData activationData = m_webPages->page(tabId, parentId);
        setWebPage(activationData.webPage);
        // Reset always height so that orentation change is taken into account.
        m_webPage->forceChrome(false);
        m_webPage->setChrome(true);
        connect(m_webPage, SIGNAL(imeNotification(int,bool,int,int,QString)),
                this, SLOT(imeNotificationChanged(int,bool,int,int,QString)), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(windowCloseRequested()), this, SLOT(closeWindow()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(urlChanged()), this, SLOT(onPageUrlChanged()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(loadingChanged()), this, SLOT(updateLoading()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(loadProgressChanged()), this, SLOT(updateLoadProgress()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(titleChanged()), this, SLOT(onPageTitleChanged()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(domContentLoadedChanged()), this, SLOT(sendVkbOpenCompositionMetrics()), Qt::UniqueConnection);
        connect(m_webPage, SIGNAL(backgroundChanged()), this, SIGNAL(backgroundChanged()), Qt::UniqueConnection);
        return activationData.activated;
    }
    return false;
}

bool DeclarativeWebContainer::alive(int tabId)
{
    return m_webPages->alive(tabId);
}

void DeclarativeWebContainer::updateMode()
{
    setTabModel(privateMode() ? m_privateTabModel.data() : m_persistentTabModel.data());
    setActiveTabData();

    // Hide currently active web page
    if (m_webPage) {
        m_webPage->setVisible(false);
        m_webPage->setOpacity(1.0);
    }

    // Reload active tab from new mode
    if (m_model->count() > 0) {
        reload(false);
    } else {
        setWebPage(NULL);
        emit contentItemChanged();
    }
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

void DeclarativeWebContainer::componentComplete()
{
    QQuickItem::componentComplete();
    if (m_initialized && !m_completed) {
        m_completed = true;
        emit completedChanged();
    }
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
    setActiveTabData();

    if (!loadActiveTab) {
        return;
    }

    // Switch to different tab.
    if (oldTabId != activeTabId) {
        reload(false);
    } else if (!m_realNavigation && isActiveTab(activeTabId) && m_webPage->backForwardNavigation()) {
        m_webPage->loadTab(m_url, false);
    }
}

void DeclarativeWebContainer::initialize()
{
    // This signal handler is responsible for activating
    // the first page.
    if (!canInitialize() || m_initialized) {
        return;
    }

    bool clearTabs = m_settingManager->clearHistoryRequested();
    int oldCount = m_model->count();

    // Clear tabs immediately from the model.
    if (clearTabs) {
        m_model->clear();
    }

    // If data was cleared when initialized and we had tabs in previous
    // session, reset tab model to unloaded state. DBManager emits
    // tabsAvailable with empty list when tabs are cleared => tab model
    // changes back to loaded and the initialize() slot gets called again.
    if (m_settingManager->initialize() && (oldCount > 0) && clearTabs) {
        m_model->setUnloaded();
        return;
    }

    // Load test
    // 1) no tabs and firstUseDone or we have incoming url, load initial url or home page to a new tab.
    // 2) model has tabs, load initial url or active tab.
    bool firstUseDone = DeclarativeWebUtils::instance()->firstUseDone();
    if (m_model->count() == 0 && (firstUseDone || !m_url.isEmpty())) {
        QString url = m_url.isEmpty() ? DeclarativeWebUtils::instance()->homePage() : m_url;
        QString title = url == m_url ? m_title : "";
        m_model->newTab(url, title);
    } else if (m_model->count() > 0) {
        const Tab &tab = m_model->activeTab();
        // Activating a web page updates url and title from the active web page.
        // So, let's figure out url and title, active page, and then set initial url and title.
        QString url = m_url.isEmpty() ? tab.url() : m_url;
        QString title = url == m_url ? m_title : tab.title();
        loadTab(tab.tabId(), url, title, true);
    }

    if (isComponentComplete() && !m_completed) {
        m_completed = true;
        emit completedChanged();
    }
    m_initialized = true;
}

void DeclarativeWebContainer::onDownloadStarted()
{
    // This is not 100% solid. A newTab is called on incoming
    // url (during browser start) if no tabs exist (waitingForNewTab). In slow network
    // connectivity one can create a new tab before downloadStarted is emitted
    // from DownloadManager. To get this to the 100%, we should add downloadStatus
    // to the QmlMozView containing status of downloading.
    if (m_model->waitingForNewTab())  {
        m_model->setWaitingForNewTab(false);
        updateUrl("");
        updateTitle("");
    } else {
        // When downloading to an existing tab refresh url,
        // title, and navigation status from the active tab. Trigger load for the tab
        // if url differs. In case browser is started with a dl url we have an "incorrect" initial url.
        QString oldUrl = m_url;
        setActiveTabData();
        if (m_url != url() || m_url != oldUrl) {
            loadTab(m_tabId, m_url, m_title, true);
        }
    }

    if (m_model->count() == 0) {
        // Download doesn't add tab to model. Mimic
        // model change in case downloading was started without
        // existing tabs.
        emit m_model->countChanged();
    }
}

void DeclarativeWebContainer::onNewTabRequested(QString url, QString title, int parentId)
{
    // TODO: Remove unused title argument.
    Q_UNUSED(title);
    // An empty tab for cleaning previous navigation status.
    Tab tab;
    updateNavigationStatus(tab);

    if (m_webPage) {
        m_webPage->setVisible(false);
        m_webPage->setOpacity(1.0);
    }

    if (activatePage(m_model->nextTabId(), false, parentId)) {
        m_webPage->setInitialUrl(url);
    }
}

void DeclarativeWebContainer::onTabsCleared()
{
    // Trigger contentItem changed and then reset title, url, and tabId.
    emit contentItemChanged();
    updateTitle("");
    updateUrl("");

    if (m_tabId != 0) {
        m_tabId = 0;
        emit tabIdChanged();
    }

    emit loadingChanged();
    setLoadProgress(0);
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
        int tabId = webPage->tabId();
        bool activeTab = isActiveTab(tabId);
        // Update initial back / forward navigation state
        if (activeTab && !webPage->urlHasChanged()) {
            const Tab &tab = m_model->activeTab();
            updateNavigationStatus(tab);
        }

        // Initial url should not be considered as navigation request that increases navigation history.
        // Cleanup this.
        bool initialLoad = !webPage->urlHasChanged();
        // Virtualized pages need to be checked from the model.
        if (webPage->boundToModel() || m_model->contains(tabId)) {
            m_model->updateUrl(tabId, activeTab, url, webPage->backForwardNavigation(), initialLoad);
        } else {
            // Adding tab to the model is delayed so that url resolved to download link do not get added
            // to the model. We should have downloadStatus(status) and linkClicked(url) signals in QmlMozView.
            // To distinguish linkClicked(url) from downloadStatus(status) the downloadStatus(status) signal
            // should not be emitted when link clicking started downloading or opened (will open) a new window.
            m_model->addTab(url, "");
        }
        webPage->bindToModel();
        webPage->setUrlHasChanged(true);

        bool wasBackForwardNavigation = webPage->backForwardNavigation();
        webPage->setBackForwardNavigation(false);

        if (activeTab && webPage == m_webPage) {
            updateUrl(url);

            if (!initialLoad && !wasBackForwardNavigation) {
                const Tab &tab = m_model->activeTab();
                updateNavigationStatus(tab);
            }
        }
    }
}

void DeclarativeWebContainer::onPageTitleChanged()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage && m_model) {
        QString url = webPage->url().toString();
        QString title = webPage->title();
        int tabId = webPage->tabId();
        bool activeTab = isActiveTab(tabId);
        m_model->updateTitle(tabId, activeTab, url, title);

        if (activeTab && webPage == m_webPage) {
            updateTitle(title);
        }
    }
}

void DeclarativeWebContainer::updateLoadProgress()
{
    if (!m_webPage || m_loadProgress == 0 && m_webPage->loadProgress() == 50) {
        return;
    }

    int progress = m_webPage->loadProgress();
    if (progress > m_loadProgress) {
        setLoadProgress(progress);
    }
}

void DeclarativeWebContainer::updateLoading()
{
    if (m_webPage && m_webPage->loading()) {
        setLoadProgress(0);
    }

    emit loadingChanged();
}

void DeclarativeWebContainer::setActiveTabData()
{
    const Tab &tab = m_model->activeTab();
#if DEBUG_LOGS
    qDebug() << "canGoBack = " << m_canGoBack << "canGoForward = " << m_canGoForward << &tab;
#endif

    updateNavigationStatus(tab);
    updateUrl(tab.url());
    updateTitle(tab.title());

    if (m_tabId != tab.tabId()) {
        m_tabId = tab.tabId();
        emit tabIdChanged();
    }
}

void DeclarativeWebContainer::updateWindowFlags()
{
    QQuickWindow *win = window();
    if (win && m_webPage) {
        static Qt::WindowFlags f = 0;
        if (f == 0) {
            f = win->flags();
        }

        if (!m_foreground) {
            win->setFlags(f | Qt::CoverWindow | Qt::FramelessWindowHint);
        } else {
            win->setFlags(f);
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
    vkbHeight *= DeclarativeWebUtils::instance()->silicaPixelRatio();
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
    if (newUrl.isEmpty() && !m_completed) {
        return;
    }

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

bool DeclarativeWebContainer::canInitialize() const
{
    return QMozContext::GetInstance()->initialized() && DownloadManager::instance()->initialized() && m_model && m_model->loaded();
}

void DeclarativeWebContainer::loadTab(int tabId, QString url, QString title, bool force)
{
    // TODO: Remove references to title.
    Q_UNUSED(title);
    if (activatePage(tabId, true) || force) {
        // Note: active pages containing a "link" between each other (parent-child relationship)
        // are not destroyed automatically e.g. in low memory notification.
        // Hence, parentId is not necessary over here.
        if (m_webPage->viewReady()) {
            m_webPage->loadTab(url, force);
        } else {
            m_webPage->setInitialUrl(url);
        }
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
