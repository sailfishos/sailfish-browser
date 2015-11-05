/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativewebpage.h"
#include "declarativewebcontainer.h"
#include "persistenttabmodel.h"
#include "privatetabmodel.h"
#include "dbmanager.h"
#include "downloadmanager.h"
#include "declarativewebutils.h"
#include "webpagefactory.h"
#include "webpages.h"

#include <QTimerEvent>
#include <QDir>
#include <QStandardPaths>
#include <QScreen>
#include <QMetaMethod>
#include <QOpenGLFunctions_ES2>
#include <QGuiApplication>
#include <qmozwindow.h>

#include <qpa/qplatformnativeinterface.h>

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

DeclarativeWebContainer::DeclarativeWebContainer(QWindow *parent)
    : QWindow(parent)
    , m_mozWindow(nullptr)
    , m_rotationHandler(0)
    , m_webPage(0)
    , m_context(0)
    , m_model(0)
    , m_webPageComponent(0)
    , m_settingManager(SettingManager::instance())
    , m_enabled(true)
    , m_foreground(true)
    , m_allowHiding(true)
    , m_popupActive(false)
    , m_portrait(true)
    , m_fullScreenMode(false)
    , m_fullScreenHeight(0.0)
    , m_imOpened(false)
    , m_toolbarHeight(0.0)
    , m_loading(false)
    , m_loadProgress(0)
    , m_completed(false)
    , m_initialized(false)
    , m_privateMode(m_settingManager->autostartPrivateBrowsing())
    , m_activeTabRendered(false)
    , m_clearSurfaceTask(0)
    , m_closing(false)
{

    QSize screenSize = QGuiApplication::primaryScreen()->size();
    resize(screenSize.width(), screenSize.height());;
    setSurfaceType(QWindow::OpenGLSurface);

    QSurfaceFormat format(requestedFormat());
    format.setRedBufferSize(5);
    format.setGreenBufferSize(6);
    format.setBlueBufferSize(5);
    format.setAlphaBufferSize(0);
    setFormat(format);

    setTitle("BrowserContent");
    setObjectName("WebView");

    QMozContext::GetInstance()->setPixelRatio(2.0);

    WebPageFactory* pageFactory = new WebPageFactory(this);
    connect(this, SIGNAL(webPageComponentChanged(QQmlComponent*)),
            pageFactory, SLOT(updateQmlComponent(QQmlComponent*)));
    m_webPages = new WebPages(pageFactory, this);
    int maxTabid = DBManager::instance()->getMaxTabId();
    m_persistentTabModel = new PersistentTabModel(maxTabid + 1, this);
    m_privateTabModel = new PrivateTabModel(maxTabid + 1001, this);

    setTabModel(privateMode() ? m_privateTabModel.data() : m_persistentTabModel.data());

    connect(DownloadManager::instance(), SIGNAL(downloadStarted()), this, SLOT(onDownloadStarted()));
    connect(QMozContext::GetInstance(), SIGNAL(onInitialized()), this, SLOT(initialize()));
    connect(QMozContext::GetInstance(), &QMozContext::lastViewDestroyed,
            this, &DeclarativeWebContainer::onLastViewDestroyed);

    QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheLocation);
    if(!dir.exists() && !dir.mkpath(cacheLocation)) {
        qWarning() << "Can't create directory "+ cacheLocation;
        return;
    }

    connect(this, SIGNAL(foregroundChanged()), this, SLOT(updateWindowFlags()));

    qApp->installEventFilter(this);
}

DeclarativeWebContainer::~DeclarativeWebContainer()
{
    // Disconnect all signal slot connections
    if (m_webPage) {
        disconnect(m_webPage, 0, 0, 0);
    }

    QMutexLocker lock(&m_clearSurfaceTaskMutex);
    if (m_clearSurfaceTask) {
        QMozContext::GetInstance()->CancelTask(m_clearSurfaceTask);
    }
}

DeclarativeWebPage *DeclarativeWebContainer::webPage() const
{
    return m_webPage;
}

QMozWindow *DeclarativeWebContainer::mozWindow() const
{
    return m_mozWindow.data();
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
            connect(m_webPage, SIGNAL(canGoBackChanged()), this, SIGNAL(canGoBackChanged()), Qt::UniqueConnection);
            connect(m_webPage, SIGNAL(canGoForwardChanged()), this, SIGNAL(canGoForwardChanged()), Qt::UniqueConnection);
            connect(m_webPage, SIGNAL(urlChanged()), this, SIGNAL(urlChanged()), Qt::UniqueConnection);
            connect(m_webPage, SIGNAL(titleChanged()), this, SIGNAL(titleChanged()), Qt::UniqueConnection);
            connect(m_webPage, SIGNAL(windowCloseRequested()), this, SLOT(closeWindow()), Qt::UniqueConnection);
            connect(m_webPage, SIGNAL(loadingChanged()), this, SLOT(updateLoading()), Qt::UniqueConnection);
            connect(m_webPage, SIGNAL(loadProgressChanged()), this, SLOT(updateLoadProgress()), Qt::UniqueConnection);
            // NB: these signals are not disconnected upon setting current m_webPage.
            connect(m_webPage, SIGNAL(urlChanged()), m_model, SLOT(onUrlChanged()), Qt::UniqueConnection);
            connect(m_webPage, SIGNAL(titleChanged()), m_model, SLOT(onTitleChanged()), Qt::UniqueConnection);

            connect(m_webPage, SIGNAL(firstPaint(int, int)), this, SLOT(onFirstPaint(int, int)), Qt::UniqueConnection);
            setActiveTabRendered(m_webPage->isPainted());
        } else {
            setActiveTabRendered(false);
        }

        emit contentItemChanged();
        emit tabIdChanged();
        emit loadingChanged();
        emit focusObjectChanged(m_webPage);
        emit canGoBackChanged();
        emit canGoForwardChanged();
        emit urlChanged();
        emit titleChanged();

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
        int oldCount = 0;
        if (m_model) {
            disconnect(m_model, 0, 0, 0);
            oldCount = m_model->count();
            m_model->setWaitingForNewTab(false);
        }

        m_model = model;
        int newCount = 0;
        if (m_model) {
            connect(m_model, SIGNAL(activeTabChanged(int,bool)), this, SLOT(onActiveTabChanged(int,bool)));
            connect(m_model, SIGNAL(activeTabChanged(int,bool)), this, SIGNAL(tabIdChanged()));
            connect(m_model, SIGNAL(loadedChanged()), this, SLOT(initialize()));
            connect(m_model, SIGNAL(tabClosed(int)), this, SLOT(releasePage(int)));
            connect(m_model, SIGNAL(newTabRequested(QString,QString,int)), this, SLOT(onNewTabRequested(QString,QString,int)));
            newCount = m_model->count();
        }
        emit tabModelChanged();
        if (m_model && oldCount != newCount) {
            emit m_model->countChanged();
        }

        // Set waiting for a tab.
        if (m_model) {
            m_model->setWaitingForNewTab(true);
        }
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

bool DeclarativeWebContainer::portrait() const
{
    return m_portrait;
}

void DeclarativeWebContainer::setPortrait(bool portrait)
{
    if (m_portrait != portrait) {
        m_portrait = portrait;
        emit portraitChanged();
    }
}

QQmlComponent* DeclarativeWebContainer::webPageComponent() const
{
    return m_webPageComponent;
}

void DeclarativeWebContainer::setWebPageComponent(QQmlComponent *qmlComponent)
{
    if (m_webPageComponent.data() != qmlComponent) {
        m_webPageComponent = qmlComponent;
        emit webPageComponentChanged(qmlComponent);
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

bool DeclarativeWebContainer::activeTabRendered() const
{
    return m_activeTabRendered;
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

bool DeclarativeWebContainer::imOpened() const
{
    return m_imOpened;
}

bool DeclarativeWebContainer::canGoForward() const
{
    return m_webPage ? m_webPage->canGoForward() : false;
}

bool DeclarativeWebContainer::canGoBack() const
{
    return m_webPage ? m_webPage->canGoBack() : false;
}

QObject *DeclarativeWebContainer::chromeWindow() const
{
    return m_chromeWindow;
}

void DeclarativeWebContainer::setChromeWindow(QObject *chromeWindow)
{
    QQuickView *quickView = qobject_cast<QQuickView*>(chromeWindow);
    if (quickView && (quickView != m_chromeWindow)) {
        m_chromeWindow = quickView;
        if (m_chromeWindow) {
            m_chromeWindow->setTransientParent(this);
            m_chromeWindow->showFullScreen();
            updateContentOrientation(m_chromeWindow->contentOrientation());
            connect(m_chromeWindow.data(), &QWindow::contentOrientationChanged,
                    this, &DeclarativeWebContainer::updateContentOrientation);
        }
        emit chromeWindowChanged();
    }
}

bool DeclarativeWebContainer::readyToPaint() const
{
     return m_mozWindow ? m_mozWindow->readyToPaint() : true;
}

void DeclarativeWebContainer::setReadyToPaint(bool ready)
{
    if (m_mozWindow && m_mozWindow->setReadyToPaint(ready)) {
        emit readyToPaintChanged();
    }
}

int DeclarativeWebContainer::tabId() const
{
    Q_ASSERT(!!m_model);
    return m_model->activeTabId();
}

QString DeclarativeWebContainer::title() const
{
    return m_webPage ? m_webPage->title() : QString();
}

QString DeclarativeWebContainer::url() const
{
    return m_webPage ? m_webPage->url().toString() : QString();
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

    if (m_webPage && m_webPage->completed()) {
        m_webPage->loadTab(url, force);
    } else if (!canInitialize()) {
        m_initialUrl = url;
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
    int activeTabId = tabId();
    if (activeTabId > 0) {
        if (force && m_webPage && m_webPage->completed() && m_webPage->tabId() == activeTabId) {
            // Reload live active tab directly.
            m_webPage->reload();
        } else {
            loadTab(m_model->activeTab(), force);
        }
    }
}

void DeclarativeWebContainer::goForward()
{
    if (m_webPage &&  m_webPage->canGoForward()) {
        DBManager::instance()->goForward(m_webPage->tabId());
        m_webPage->goForward();
    }
}

void DeclarativeWebContainer::goBack()
{
    if (m_webPage && m_webPage->canGoBack()) {
        DBManager::instance()->goBack(m_webPage->tabId());
        m_webPage->goBack();
    }
}

bool DeclarativeWebContainer::activatePage(const Tab& tab, bool force, int parentId)
{
    if (!m_model) {
        return false;
    }

    m_webPages->initialize(this);
    if ((m_model->loaded() || force) && tab.tabId() > 0 && m_webPages->initialized() && m_webPageComponent) {
        WebPageActivationData activationData = m_webPages->page(tab, parentId);
        setActiveTabRendered(false);
        setWebPage(activationData.webPage);
        // Reset always height so that orentation change is taken into account.
        m_webPage->forceChrome(false);
        m_webPage->setChrome(true);
        return activationData.activated;
    }
    return false;
}

int DeclarativeWebContainer::findParentTabId(int tabId) const
{
    if (m_webPages) {
        return m_webPages->parentTabId(tabId);
    }
    return 0;
}

void DeclarativeWebContainer::updateMode()
{
    setTabModel(privateMode() ? m_privateTabModel.data() : m_persistentTabModel.data());
    emit tabIdChanged();

    // Reload active tab from new mode
    if (m_model->count() > 0) {
        reload(false);
    } else {
        setWebPage(NULL);
        emit contentItemChanged();
    }
}

void DeclarativeWebContainer::setActiveTabRendered(bool rendered)
{
    if (m_activeTabRendered != rendered) {
        m_activeTabRendered = rendered;
        emit activeTabRenderedChanged();
    }
}

bool DeclarativeWebContainer::postClearWindowSurfaceTask()
{
    QMutexLocker lock(&m_clearSurfaceTaskMutex);
    if (m_clearSurfaceTask) {
        return true;
    }
    m_clearSurfaceTask = QMozContext::GetInstance()->PostCompositorTask(
        &DeclarativeWebContainer::clearWindowSurfaceTask, this);
    return m_clearSurfaceTask != 0;
}

void DeclarativeWebContainer::clearWindowSurfaceTask(void *data)
{
    DeclarativeWebContainer* that = static_cast<DeclarativeWebContainer*>(data);
    QMutexLocker lock(&that->m_clearSurfaceTaskMutex);
    that->clearWindowSurface();
    that->m_clearSurfaceTask = 0;
}

void DeclarativeWebContainer::clearWindowSurface()
{
    Q_ASSERT(m_context);
    // The GL context should always be used from the same thread in which it was created.
    Q_ASSERT(m_context->thread() == QThread::currentThread());
    m_context->makeCurrent(this);
    QOpenGLFunctions_ES2* funcs = m_context->versionFunctions<QOpenGLFunctions_ES2>();
    Q_ASSERT(funcs);
    funcs->glClearColor(1.0, 1.0, 1.0, 0.0);
    funcs->glClear(GL_COLOR_BUFFER_BIT);
    m_context->swapBuffers(this);
}

void DeclarativeWebContainer::dumpPages() const
{
    m_webPages->dumpPages();
}

QObject *DeclarativeWebContainer::focusObject() const
{
    return m_webPage ? m_webPage : QWindow::focusObject();
}

bool DeclarativeWebContainer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Close && m_mozWindow) {
        m_mozWindow->suspendRendering();
        m_closing = true;
        m_webPages->clear();
    }

    // Emit chrome exposed when both chrome window and browser window has been exposed. This way chrome
    // window can be raised to the foreground if needed.
    static bool hasExposedChrome = false;
    if (!hasExposedChrome && event->type() == QEvent::Show && m_chromeWindow && m_chromeWindow->isExposed() && isExposed()) {
        emit chromeExposed();
        hasExposedChrome = true;
    }

    return QObject::eventFilter(obj, event);
}

bool DeclarativeWebContainer::event(QEvent *event)
{
    QPlatformWindow *windowHandle;
    if (event->type() == QEvent::PlatformSurface
                && static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated
                && (windowHandle = handle())) {
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        native->setWindowProperty(windowHandle, QStringLiteral("BACKGROUND_VISIBLE"), false);
        native->setWindowProperty(windowHandle, QStringLiteral("HAS_CHILD_WINDOWS"), true);
    }
    return QWindow::event(event);
}

void DeclarativeWebContainer::exposeEvent(QExposeEvent*)
{
    // Filter out extra expose event spam. We often get 3-4 expose events
    // in a row. For all of them isExposed returns true. We only want to
    // clear the compositing surface once in such case.
    static bool alreadyExposed = false;

    if (isExposed() && !alreadyExposed) {
        if (m_chromeWindow) {
            m_chromeWindow->update();
        }

        if (m_webPage) {
            m_webPage->update();
        } else {
            if (postClearWindowSurfaceTask()) {
                alreadyExposed = true;
                return;
            }

            // The compositor thread has not been created on gecko side, yet.
            // We can use temporary GL context to clear the contents of the
            // surface.
            QMutexLocker lock(&m_contextMutex);
            if (!m_context) {
                QOpenGLContext context;
                context.setFormat(requestedFormat());
                context.create();

                m_context = &context;
                clearWindowSurface();
                m_context = 0;

                context.doneCurrent();
            }
        }
    }
    alreadyExposed = isExposed();
}

void DeclarativeWebContainer::touchEvent(QTouchEvent *event)
{
    if (!m_rotationHandler) {
        qWarning() << "Cannot deliver touch events without rotationHandler";
        return;
    }

    if (m_webPage && m_enabled) {
        QList<QTouchEvent::TouchPoint> touchPoints = event->touchPoints();
        QTouchEvent mappedTouchEvent = *event;

        for (int i = 0; i < touchPoints.count(); ++i) {
            QPointF pt = m_rotationHandler->mapFromScene(touchPoints.at(i).pos());
            touchPoints[i].setPos(pt);
        }

        mappedTouchEvent.setTouchPoints(touchPoints);
        m_webPage->touchEvent(&mappedTouchEvent);
    }
}

QVariant DeclarativeWebContainer::inputMethodQuery(Qt::InputMethodQuery property) const
{
    if (m_webPage && m_enabled) {
        return m_webPage->inputMethodQuery(property);
    }
    return QVariant();
}

void DeclarativeWebContainer::inputMethodEvent(QInputMethodEvent *event)
{
    if (m_webPage && m_enabled) {
        m_webPage->inputMethodEvent(event);
    }
}

void DeclarativeWebContainer::keyPressEvent(QKeyEvent *event)
{
    if (m_webPage && m_enabled) {
        m_webPage->keyPressEvent(event);
    }
}

void DeclarativeWebContainer::keyReleaseEvent(QKeyEvent *event)
{
    if (m_webPage && m_enabled) {
        m_webPage->keyReleaseEvent(event);
    }
}

void DeclarativeWebContainer::focusInEvent(QFocusEvent *event)
{
    if (m_webPage && m_enabled) {
        m_webPage->focusInEvent(event);
    }
}

void DeclarativeWebContainer::focusOutEvent(QFocusEvent *event)
{
    if (m_webPage && m_enabled) {
        m_webPage->focusOutEvent(event);
    }
}

void DeclarativeWebContainer::timerEvent(QTimerEvent *event)
{
    if (m_webPage && m_enabled) {
        m_webPage->timerEvent(event);
    }
}

void DeclarativeWebContainer::classBegin()
{
}

void DeclarativeWebContainer::componentComplete()
{
    showFullScreen();

    if (m_initialized && !m_completed) {
        m_completed = true;
        emit completedChanged();
    }
}

void DeclarativeWebContainer::updateContentOrientation(Qt::ScreenOrientation orientation)
{
    if (m_mozWindow) {
        m_mozWindow->setContentOrientation(orientation);
    }
    reportContentOrientationChange(orientation);
}

qreal DeclarativeWebContainer::contentHeight() const
{
    if (m_webPage) {
        return m_webPage->contentHeight();
    } else {
        return 0.0;
    }
}

void DeclarativeWebContainer::onActiveTabChanged(int activeTabId, bool loadActiveTab)
{
    if (activeTabId <= 0) {
        return;
    }

    if (!loadActiveTab) {
        return;
    }

    reload(false);
}

void DeclarativeWebContainer::initialize()
{
    if (QMozContext::GetInstance()->initialized() && !m_mozWindow) {
        m_mozWindow.reset(new QMozWindow);
        m_mozWindow->setSize(QWindow::size());
        connect(m_mozWindow.data(), &QMozWindow::requestGLContext,
                this, &DeclarativeWebContainer::createGLContext, Qt::DirectConnection);
        connect(m_mozWindow.data(), &QMozWindow::drawUnderlay,
                this, &DeclarativeWebContainer::drawUnderlay, Qt::DirectConnection);
        if (m_chromeWindow) {
            updateContentOrientation(m_chromeWindow->contentOrientation());
        }
    }

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
    if (m_model->count() == 0 && (firstUseDone || !m_initialUrl.isEmpty())) {
        QString url = m_initialUrl.isEmpty() ? DeclarativeWebUtils::instance()->homePage() : m_initialUrl;
        QString title = "";
        m_model->newTab(url, title);
    } else if (m_model->count() > 0) {
        Tab tab = m_model->activeTab();
        if (!m_initialUrl.isEmpty()) {
            tab.setUrl(m_initialUrl);
        }
        loadTab(tab, true);
    }

    if (!m_completed) {
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
    } else {
        // In case browser is started with a dl url we have an "incorrect" initial url.
        // Emit urlChange() in order to trigger restoreHistory()
        emit m_webPage->urlChanged();
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
    Tab tab;
    tab.setTabId(m_model->nextTabId());
    if (activatePage(tab, false, parentId)) {
        m_webPage->loadTab(url, false);
    }
}

void DeclarativeWebContainer::releasePage(int tabId)
{
    if (m_webPages) {
        m_webPages->release(tabId);
        // Successfully destroyed. Emit relevant property changes.
        if (m_model->count() == 0) {
            setWebPage(NULL);
            postClearWindowSurfaceTask();
        }
    }
}

void DeclarativeWebContainer::closeWindow()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    if (webPage && m_model) {
        int parentPageTabId = findParentTabId(webPage->tabId());
        // Closing only allowed if window was created by script
        if (parentPageTabId > 0) {
            m_model->activateTabById(parentPageTabId);
            m_model->removeTabById(webPage->tabId(), isActiveTab(webPage->tabId()));
        }
    }
}

void DeclarativeWebContainer::updateLoadProgress()
{
    if (!m_webPage || (m_loadProgress == 0 && m_webPage->loadProgress() == 50)) {
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

void DeclarativeWebContainer::onFirstPaint(int, int)
{
    updateActiveTabRendered();
}

void DeclarativeWebContainer::updateActiveTabRendered()
{
    setActiveTabRendered(true);
    // One frame rendered so let's disconnect.
    disconnect(m_mozWindow.data(), &QMozWindow::compositingFinished,
               this, &DeclarativeWebContainer::updateActiveTabRendered);
}

void DeclarativeWebContainer::onLastViewDestroyed()
{
    if (m_closing) {
        m_mozWindow.reset();
    }
}

void DeclarativeWebContainer::updateWindowFlags()
{
    if (m_webPage) {
        static Qt::WindowFlags f = 0;
        if (f == 0) {
            f = flags();
        }

        if (!m_foreground) {
            setFlags(f | Qt::CoverWindow | Qt::FramelessWindowHint);
        } else {
            setFlags(f);
        }
    }
}

void DeclarativeWebContainer::updatePageFocus(bool focus)
{
    if (m_webPage) {
        if (focus) {
            QFocusEvent focusEvent(QEvent::FocusIn);
            m_webPage->focusInEvent(&focusEvent);
        } else {
            QFocusEvent focusEvent(QEvent::FocusOut);
            m_webPage->focusOutEvent(&focusEvent);
        }
    }
}

bool DeclarativeWebContainer::canInitialize() const
{
    return QMozContext::GetInstance()->initialized() && m_model && m_model->loaded();
}

void DeclarativeWebContainer::loadTab(const Tab& tab, bool force)
{
    if (activatePage(tab, true) || force) {
        // Note: active pages containing a "link" between each other (parent-child relationship)
        // are not destroyed automatically e.g. in low memory notification.
        // Hence, parentId is not necessary over here.
        m_webPage->loadTab(tab.url(), force);
    }
}

void DeclarativeWebContainer::createGLContext()
{
    QMutexLocker lock(&m_contextMutex);
    if (!m_context) {
        m_context = new QOpenGLContext();
        m_context->setFormat(requestedFormat());
        m_context->create();
        m_context->makeCurrent(this);
        initializeOpenGLFunctions();
    } else {
        m_context->makeCurrent(this);
    }

    if (!m_activeTabRendered) {
        clearWindowSurface();
    }
}

void DeclarativeWebContainer::drawUnderlay()
{
    Q_ASSERT(m_context);

    if (!m_webPage) {
       return;
    }

    m_context->makeCurrent(this);
    QOpenGLFunctions_ES2* funcs = m_context->versionFunctions<QOpenGLFunctions_ES2>();
    if (funcs) {
        QColor bgColor = m_webPage->bgcolor();
        funcs->glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), 0.0);
        funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}
