/*
 * Copyright (c) 2013 - 2021 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "declarativewebpage.h"
#include "declarativewebcontainer.h"
#include "persistenttabmodel.h"
#include "privatetabmodel.h"
#include "dbmanager.h"
#include "downloadmanager.h"
#include "declarativewebutils.h"
#include "webpagefactory.h"
#include "webpages.h"
#include "browserpaths.h"
#include "browserappinfo.h"
#include "logging.h"
#include "declarativehistorymodel.h"
#include "closeeventfilter.h"
#include "settingmanager.h"

#include <webengine.h>
#include <QTimerEvent>
#include <QScreen>
#include <QMetaMethod>
#include <QOpenGLFunctions_ES2>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QGuiApplication>
#include <QResizeEvent>
#include <qmozwindow.h>
#include <qmozsecurity.h>

#include <MDConfItem>

#include <qpa/qplatformnativeinterface.h>
#include <libsailfishpolicy/policyvalue.h>

#include <QDBusConnection>
#include <dsme/dsme_dbus_if.h>

#include <EGL/egl.h>
#include <GLES2/gl2ext.h>

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

static const bool gForceLandscapeToPortrait = !qgetenv("BROWSER_FORCE_LANDSCAPE_TO_PORTRAIT").isEmpty();
static const auto ABOUT_BLANK = QStringLiteral("about:blank");

static DeclarativeWebContainer *s_instance = nullptr;

static QColor opaqueSurfaceColor(const QColor &color)
{
    QColor normalized = color.isValid() ? color : QColor(Qt::black);
    normalized.setAlpha(255);
    return normalized;
}

static void setGLClearColor(QOpenGLFunctions_ES2 *functions, const QColor &color)
{
    const QColor normalized = opaqueSurfaceColor(color);
    functions->glClearColor(normalized.redF(), normalized.greenF(),
                            normalized.blueF(), 1.0f);
}

static GLenum glTextureTarget(QMozTextureTarget textureTarget)
{
    return textureTarget == QMozTextureTarget::ExternalOES
            ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;
}

static void updateTextureCoordinates(Qt::ScreenOrientation orientation, GLfloat *coordinates)
{
    // WebRender renders into a GL framebuffer, whose EGLImage has a
    // bottom-left texture origin. Qt's window coordinates are top-left based.
    const GLfloat topLeft[] = { 0.0f, 1.0f };
    const GLfloat bottomLeft[] = { 0.0f, 0.0f };
    const GLfloat topRight[] = { 1.0f, 1.0f };
    const GLfloat bottomRight[] = { 1.0f, 0.0f };

    const int rotation = qApp->primaryScreen()->angleBetween(
                orientation, qApp->primaryScreen()->primaryOrientation());
    const GLfloat *corners[] = { topLeft, bottomLeft, topRight, bottomRight };

    switch (rotation) {
    case 90:
        corners[0] = bottomLeft;
        corners[1] = bottomRight;
        corners[2] = topLeft;
        corners[3] = topRight;
        break;
    case 180:
        corners[0] = bottomRight;
        corners[1] = topRight;
        corners[2] = bottomLeft;
        corners[3] = topLeft;
        break;
    case 270:
        corners[0] = topRight;
        corners[1] = topLeft;
        corners[2] = bottomRight;
        corners[3] = bottomLeft;
        break;
    default:
        break;
    }

    for (int i = 0; i < 4; ++i) {
        coordinates[i * 2] = corners[i][0];
        coordinates[i * 2 + 1] = corners[i][1];
    }
}

static bool isLandscapeOrientation(Qt::ScreenOrientation orientation)
{
    return orientation == Qt::LandscapeOrientation
            || orientation == Qt::InvertedLandscapeOrientation;
}

static QSizeF sizeForOrientation(const QSizeF &size, Qt::ScreenOrientation orientation)
{
    if (!size.isValid() || size.width() <= 0.0 || size.height() <= 0.0) {
        return size;
    }

    const bool sizeLandscape = size.width() >= size.height();
    if (sizeLandscape != isLandscapeOrientation(orientation)) {
        return QSizeF(size.height(), size.width());
    }

    return size;
}

static void applyTextureRect(const QRectF &rect, GLfloat *coordinates)
{
    if (rect.left() <= 0.0 && rect.top() <= 0.0
            && rect.width() >= 1.0 && rect.height() >= 1.0) {
        return;
    }

    for (int i = 0; i < 4; ++i) {
        const GLfloat u = coordinates[i * 2];
        const GLfloat v = coordinates[i * 2 + 1];
        const GLfloat topOriginY = 1.0f - v;
        coordinates[i * 2] = rect.left() + u * rect.width();
        coordinates[i * 2 + 1] = 1.0f - (rect.top() + topOriginY * rect.height());
    }
}

static QImage readCurrentFramebuffer(const QSize &size)
{
    while (glGetError()) {}

    QImage image(size, QImage::Format_RGB32);
    glReadPixels(0, 0, size.width(), size.height(), GL_BGRA_EXT,
                 GL_UNSIGNED_BYTE, image.bits());
    if (glGetError() == GL_NO_ERROR) {
        return image.mirrored();
    }

    QImage rgbaImage(size, QImage::Format_RGBX8888);
    glReadPixels(0, 0, size.width(), size.height(), GL_RGBA,
                 GL_UNSIGNED_BYTE, rgbaImage.bits());
    if (glGetError() == GL_NO_ERROR) {
        return rgbaImage.mirrored();
    }

    return QImage();
}

DeclarativeWebContainer::DeclarativeWebContainer(QWindow *parent)
    : QWindow(parent)
{
    Q_ASSERT(!s_instance);

    QSize screenSize = QGuiApplication::primaryScreen()->size();
    resize(screenSize.width(), screenSize.height());
    setSurfaceType(QWindow::OpenGLSurface);

    QSurfaceFormat format(requestedFormat());
    format.setRedBufferSize(5);
    format.setGreenBufferSize(6);
    format.setBlueBufferSize(5);
    format.setAlphaBufferSize(0);
    setFormat(format);

    setTitle("BrowserContent");
    setObjectName("WebView");


    MDConfItem privatebrowsingAutostart(QStringLiteral("/apps/sailfish-browser/settings/browser_privatebrowsing_autostart"));

    if (!browserEnabled() || privatebrowsingAutostart.value(QVariant(false)).toBool()) m_privateMode = true;

    WebPageFactory* pageFactory = new WebPageFactory(this);
    connect(this, &DeclarativeWebContainer::webPageComponentChanged,
            pageFactory, &WebPageFactory::updateQmlComponent);
    m_webPages = new WebPages(pageFactory, this);
    int maxTabid = DBManager::instance()->getMaxTabId();
    m_persistentTabModel = new PersistentTabModel(maxTabid + 1, this);
    m_privateTabModel = new PrivateTabModel(maxTabid + 1001, this);

    setTabModel((BrowserAppInfo::captivePortal() || m_privateMode) ? m_privateTabModel.data()
                                                                   : m_persistentTabModel.data());

    connect(DownloadManager::instance(), &DownloadManager::downloadStarted,
            this, &DeclarativeWebContainer::onDownloadStarted);
    SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();
    connect(webEngine, &SailfishOS::WebEngine::initialized,
            this, &DeclarativeWebContainer::initialize);
    connect(webEngine, &SailfishOS::WebEngine::lastViewDestroyed,
            this, &DeclarativeWebContainer::onLastViewDestroyed);
    connect(webEngine, &SailfishOS::WebEngine::lastWindowDestroyed,
            this, &DeclarativeWebContainer::onLastWindowDestroyed);

    QString cacheLocation = BrowserPaths::cacheLocation();
    if (cacheLocation.isNull()) {
        return;
    }

    connect(this, &DeclarativeWebContainer::foregroundChanged,
            this, &DeclarativeWebContainer::updateWindowFlags);

    qApp->installEventFilter(this);

    m_closeEventFilter = new CloseEventFilter(DownloadManager::instance(), this);
    s_instance = this;

    // Configure the "hidden tab" callback
    m_hiddenTabTimer.setSingleShot(true);
    m_hiddenTabTimer.setInterval(0);
    connect(&m_hiddenTabTimer, &QTimer::timeout,
            this, &DeclarativeWebContainer::restorePreviousTabDelayed);

    QDBusConnection::systemBus().connect(dsme_service, dsme_sig_path, dsme_sig_interface, dsme_state_change_ind,
                                         this, SLOT(dsmeStateChange(QString)));
}

DeclarativeWebContainer::~DeclarativeWebContainer()
{
    // Disconnect all signal slot connections
    if (m_webPage) {
        disconnect(m_webPage, 0, 0, 0);
    }

    disconnect(&m_hiddenTabTimer, &QTimer::timeout,
               this, &DeclarativeWebContainer::restorePreviousTabDelayed);

    if (m_context) {
        if (m_context->makeCurrent(this)) {
            if (m_frameTexture) {
                glDeleteTextures(1, &m_frameTexture);
                m_frameTexture = 0;
            }
            delete m_texture2DProgram;
            m_texture2DProgram = nullptr;
            delete m_externalTextureProgram;
            m_externalTextureProgram = nullptr;
            m_context->doneCurrent();
        }
        delete m_context;
        m_context = nullptr;
    }
}

DeclarativeWebContainer *DeclarativeWebContainer::instance()
{
    Q_ASSERT(s_instance);
    return s_instance;
}

DeclarativeWebPage *DeclarativeWebContainer::webPage() const
{
    return m_webPage;
}

QMozWindow *DeclarativeWebContainer::mozWindow() const
{
    return m_mozWindow.data();
}

void DeclarativeWebContainer::setWebPage(DeclarativeWebPage *webPage, bool triggerSignals)
{
    if (m_webPage != webPage || triggerSignals) {
        const bool activePageChanged = (m_webPage != webPage);

        // Disconnect previous page.
        if (m_webPage) {
            m_webPage->disconnect(this);
        }

        m_webPage = webPage;
        // Mark as not rendered when ever tab is changed.
        setActiveTabRendered(false);
        if (activePageChanged) {
            const bool waitForActiveTabLoad = webPage != nullptr
                    && (!webPage->loaded() || webPage->loading());
            m_waitingForActiveTabFrame = webPage != nullptr;
            m_waitingForActiveTabLoad = waitForActiveTabLoad;
            m_waitingForActiveTabFirstPaint = webPage != nullptr
                    && !webPage->isPainted();
            m_activeTabCompositesToSkip = webPage
                    && !m_waitingForActiveTabFirstPaint ? 1 : 0;
            if (m_mozWindow) {
                m_mozWindow->clearPlatformImage();
            }
            clearSurface();
        }

        if (m_webPage) {
            connect(m_webPage.data(), &DeclarativeWebPage::canGoBackChanged,
                    this, &DeclarativeWebContainer::canGoBackChanged, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::canGoForwardChanged,
                    this, &DeclarativeWebContainer::canGoForwardChanged, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::urlChanged,
                    this, &DeclarativeWebContainer::urlChanged, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::titleChanged,
                    this, &DeclarativeWebContainer::titleChanged, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::windowCloseRequested,
                    this, &DeclarativeWebContainer::closeWindow, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::loadingChanged,
                    this, &DeclarativeWebContainer::updateLoading, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::loadProgressChanged,
                    this, &DeclarativeWebContainer::updateLoadProgress, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::contentOrientationChanged,
                    this, &DeclarativeWebContainer::handleContentOrientationChanged, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::securityChanged,
                    this, &DeclarativeWebContainer::securityChanged, Qt::UniqueConnection);

            // NB: these signals are not disconnected upon setting current m_webPage.
            connect(m_webPage.data(), &DeclarativeWebPage::updateUrl,
                    m_model.data(), &DeclarativeTabModel::onUrlChanged, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::desktopModeChanged,
                    m_model.data(), &DeclarativeTabModel::onDesktopModeChanged, Qt::UniqueConnection);
            connect(m_webPage.data(), &DeclarativeWebPage::titleChanged,
                    m_model.data(), &DeclarativeTabModel::onTitleChanged, Qt::UniqueConnection);

            // Track when the active tab is ready for thumbnail capture.
            connect(m_webPage.data(), &QMozOpenGLWebPage::domContentLoadedChanged,
                    this, &DeclarativeWebContainer::updateActiveTabRendered, Qt::UniqueConnection);
            connect(m_webPage.data(), &QMozOpenGLWebPage::firstPaint,
                    this, &DeclarativeWebContainer::handleActiveTabFirstPaint, Qt::UniqueConnection);

            connect(m_webPage.data(), &DeclarativeWebPage::neterror, [this]() {
                if (m_historyModel)
                    m_historyModel->remove(m_webPage->url().toString());
            });

            connect(m_webPage.data(), &DeclarativeWebPage::updateUrl, [this]() {
                if (!BrowserAppInfo::captivePortal() && !m_privateMode && m_historyModel)
                    m_historyModel->add(m_webPage->url().toString(), QString());
            });

            if (m_webPage->completed() && m_webPage->active() && (m_webPage->isPainted() || m_webPage->domContentLoaded())) {
                m_webPage->update();
            }
        }

        emit contentItemChanged();
        emit tabIdChanged();
        emit loadingChanged();
        emit focusObjectChanged(m_webPage);
        emit canGoBackChanged();
        emit canGoForwardChanged();
        emit urlChanged();
        emit titleChanged();
        emit securityChanged();

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
        }

        m_model = model;
        int newCount = 0;
        if (m_model) {
            connect(m_model.data(), &DeclarativeTabModel::activeTabChanged,
                    this, &DeclarativeWebContainer::onActiveTabChanged);
            connect(m_model.data(), &DeclarativeTabModel::activeTabChanged,
                    this, &DeclarativeWebContainer::tabIdChanged);
            connect(m_model.data(), &DeclarativeTabModel::loadedChanged,
                    this, &DeclarativeWebContainer::initialize);
            connect(m_model.data(), &DeclarativeTabModel::tabClosed,
                    this, &DeclarativeWebContainer::releasePage);
            connect(m_model.data(), &DeclarativeTabModel::newTabRequested,
                    this, &DeclarativeWebContainer::onNewTabRequested);
            newCount = m_model->count();
        }
        emit tabModelChanged();
        if (m_model && oldCount != newCount) {
            emit m_model->countChanged();
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

QQmlComponent* DeclarativeWebContainer::webPageComponent() const
{
    return m_webPageComponent;
}

void DeclarativeWebContainer::setWebPageComponent(QQmlComponent *qmlComponent)
{
    if (m_webPageComponent.data() != qmlComponent) {
        m_webPageComponent = qmlComponent;
        emit webPageComponentChanged(qmlComponent);
        initialize();
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

bool DeclarativeWebContainer::canGoForward() const
{
    return m_webPage && m_webPage->canGoForward();
}

bool DeclarativeWebContainer::canGoBack() const
{
    return m_webPage && m_webPage->canGoBack();
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
    if (m_mozWindow) {
        bool changed = m_mozWindow->setReadyToPaint(ready);

        if (ready) {
            m_mozWindow->resumeRendering();
        } else {
            m_mozWindow->suspendRendering();
        }

        if (changed) {
            emit readyToPaintChanged();
        }
    }
}

QRectF DeclarativeWebContainer::webContentRect() const
{
    return m_webContentRect;
}

void DeclarativeWebContainer::setWebContentRect(const QRectF &rect)
{
    if (m_webContentRect == rect) {
        return;
    }

    m_webContentRect = rect;
    updateMozWindowSize();

    if (isExposed() && m_frameTexture) {
        renderCompositedFrame();
    }

    emit webContentRectChanged();
}

QColor DeclarativeWebContainer::webContentBackgroundColor() const
{
    return m_webContentBackgroundColor;
}

void DeclarativeWebContainer::setWebContentBackgroundColor(const QColor &color)
{
    const QColor normalized = opaqueSurfaceColor(color);
    if (m_webContentBackgroundColor == normalized) {
        return;
    }

    m_webContentBackgroundColor = normalized;

    if (isExposed()) {
        if (m_frameTexture) {
            renderCompositedFrame();
        } else if (m_context) {
            clearWindowSurface();
        }
    }

    emit webContentBackgroundColorChanged();
}

Qt::ScreenOrientation DeclarativeWebContainer::pendingWebContentOrientation() const
{
    return m_mozWindow ? m_mozWindow->pendingOrientation() : Qt::PortraitOrientation;
}

QMozSecurity *DeclarativeWebContainer::security() const
{
    return m_webPage ? m_webPage->security() : nullptr;
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

void DeclarativeWebContainer::load(const QString &url, bool force, bool fromExternal)
{
    QString tmpUrl = url;
    if (tmpUrl.isEmpty() || !browserEnabled()) {
        tmpUrl = ABOUT_BLANK;
    }

    if (!canInitialize()) {
        m_initialUrl = tmpUrl;
        m_fromExternal = fromExternal;
    } else if (m_webPage && m_webPage->completed()) {
        if (loading()) {
            m_webPage->stop();
        }
        m_webPage->loadTab(tmpUrl, force, fromExternal);
        Tab *tab = m_model->getTab(m_webPage->tabId());
        if (tab) {
            tab->setRequestedUrl(tmpUrl);
        }
    } else if (m_model && m_model->count() == 0) {
        // Browser running all tabs are closed.
        m_model->newTab(tmpUrl, fromExternal);
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
            loadTab(m_model->activeTab(), force, false);
        }
    }
}

void DeclarativeWebContainer::goForward()
{
    if (m_webPage && m_webPage->canGoForward()) {
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

void DeclarativeWebContainer::closeTab(int tabId)
{
    m_model->removeTabById(tabId, false);
}

int DeclarativeWebContainer::activateTab(int tabId, const QString &url)
{
    return requestTabWithOwner(tabId, url, 0);
}

int DeclarativeWebContainer::requestTabWithOwner(int tabId, const QString &url, uint ownerPid)
{
    bool activated = m_model->activateTabById(tabId);
    if (!activated) {
        tabId = m_model->newTab(url, false);
        if (ownerPid) {
            m_tabOwners.insert(tabId, ownerPid);
        }
    } else {
        load(url, true, false);
    }

    return tabId;
}

void DeclarativeWebContainer::requestTabWithOwnerAsync(int tabId, const QString &url, uint ownerPid, void *context)
{
    // We should only create or activate tabs once the model has loaded
    if (m_model->loaded()) {
        // The tab model has already loaded, so we can go ahead and create the tab
        int activatedTab = requestTabWithOwner(tabId, url, ownerPid);
        emit requestTabWithOwnerAsyncResult(activatedTab, context);
    } else {
        // The model has yet to load, so queue creation of the tab
        QMetaObject::Connection * const connection = new QMetaObject::Connection;
        *connection = connect(m_model.data(), &DeclarativeTabModel::loadedChanged,
                              this, [this, tabId, url, ownerPid, context, connection]() {
            // We assume that m_model->loaded() is now set to true
            int activatedTab = requestTabWithOwner(tabId, url, ownerPid);
            qCDebug(lcCoreLog) << "Delaying tab request created tabId:" << activatedTab;
            emit requestTabWithOwnerAsyncResult(activatedTab, context);
            // Single-shot connection
            QObject::disconnect(*connection);
            delete connection;
        });
        qCDebug(lcCoreLog) << "Tab requested while loading, delaying request on tabId:" << tabId;
    }
}

uint DeclarativeWebContainer::tabOwner(int tabId) const
{
    return m_tabOwners.value(tabId);
}

void DeclarativeWebContainer::releaseActiveTabOwnership()
{
    qCDebug(lcCoreLog) << "Releasing ownership of active tab";
    if (m_model) {
        m_tabOwners.remove(m_model->activeTabId());
    }
}

bool DeclarativeWebContainer::activatePage(const Tab& tab, bool force, bool fromExternal)
{
    if (!m_initialized) {
        m_initialUrl = tab.requestedUrl();
        m_fromExternal = fromExternal;
        return false;
    }

    m_webPages->initialize(this);
    if ((m_model->loaded() || force) && tab.tabId() > 0 && m_webPages->isInitialized() && m_webPageComponent) {
        WebPageActivationData activationData = m_webPages->page(tab);
        setWebPage(activationData.webPage);
        // Reset always height so that orientation change is taken into account.
        m_webPage->forceChrome(false);
        m_webPage->setChrome(true);
        if (m_webPage->loaded()) {
            m_webPage->update();
        }

        return activationData.activated;
    }
    return false;
}

QImage DeclarativeWebContainer::grabContentImage(const QSize &size)
{
    if (!isExposed() || !m_mozWindow || !m_webPage || !m_webPage->active()) {
        return QImage();
    }

    const QSize targetSize = size.isEmpty() ? webContentSize() : size;
    if (!targetSize.isValid() || targetSize.width() <= 0 || targetSize.height() <= 0) {
        qWarning() << "Cannot grab browser WebRender frame with invalid size" << targetSize;
        return QImage();
    }

    if (!ensureRenderContext()) {
        return QImage();
    }

    QSize textureSize;
    QMozTextureTarget textureTarget;
    if (!bindWebRenderFrameTexture(&textureSize, &textureTarget)
            || !ensureTextureProgram(textureTarget)) {
        qWarning() << "No WebRender EGLImage available for browser frame grab";
        return QImage();
    }

    QOpenGLFramebufferObject frameBuffer(targetSize);
    if (!frameBuffer.isValid() || !frameBuffer.bind()) {
        qWarning() << "Failed to create browser WebRender frame grab buffer" << targetSize;
        return QImage();
    }

    glViewport(0, 0, targetSize.width(), targetSize.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    const QColor clearColor = opaqueSurfaceColor(m_webContentBackgroundColor);
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const Qt::ScreenOrientation contentOrientation = m_mozWindow->contentOrientation();
    const QRectF contentRect = m_webPage->contentRect();
    const qreal contentResolution = m_webPage->resolution();
    const qreal toolbarHeight = qMax(m_webPage->toolbarHeight(),
                                     static_cast<qreal>(m_webPage->dynamicToolbarHeight()));
    QSizeF drawSize = contentResolution > 0.0
            ? contentRect.size() * contentResolution
            : QSizeF();
    if (toolbarHeight > 0.0 && drawSize.height() > toolbarHeight) {
        drawSize.setHeight(drawSize.height() - toolbarHeight);
    }
    if (!drawSize.isValid() || drawSize.width() <= 0.0 || drawSize.height() <= 0.0) {
        drawSize = sizeForOrientation(m_mozWindow->size(), contentOrientation);
    }
    if (!drawSize.isValid() || drawSize.width() <= 0.0 || drawSize.height() <= 0.0) {
        drawSize = sizeForOrientation(webContentSize(), contentOrientation);
    }
    if (!drawSize.isValid() || drawSize.width() <= 0.0 || drawSize.height() <= 0.0) {
        drawSize = sizeForOrientation(textureSize, contentOrientation);
    }
    if (!drawSize.isValid() || drawSize.width() <= 0.0 || drawSize.height() <= 0.0) {
        drawSize = targetSize;
    }
    const QSizeF sourceSize = drawSize;
    drawSize.scale(targetSize, Qt::KeepAspectRatioByExpanding);
    const Qt::ScreenOrientation grabOrientation = qApp->primaryScreen()
            ? qApp->primaryScreen()->primaryOrientation()
            : contentOrientation;

    QRectF textureRect(0.0, 0.0, 1.0, 1.0);
    if (textureSize.width() > 0 && textureSize.height() > 0) {
        textureRect.setWidth(qBound<qreal>(0.0, sourceSize.width() / textureSize.width(), 1.0));
        textureRect.setHeight(qBound<qreal>(0.0, sourceSize.height() / textureSize.height(), 1.0));
    }

    const QRectF targetRect(0.0, 0.0, drawSize.width(), drawSize.height());
    QImage image;
    if (drawWebRenderFrame(targetRect, QSizeF(targetSize), grabOrientation, textureTarget,
                           textureRect)) {
        image = readCurrentFramebuffer(targetSize);
    } else {
        qWarning() << "Browser WebRender frame grab draw failed"
                   << "textureSize" << textureSize << "targetSize" << targetSize;
    }

    frameBuffer.release();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return image;
}

int DeclarativeWebContainer::tabId(uint32_t uniqueId) const
{
    if (m_webPages) {
        return m_webPages->tabId(uniqueId);
    }
    return 0;
}

int DeclarativeWebContainer::previouslyUsedTabId() const
{
    if (m_webPages) {
        return m_webPages->previouslyUsedTabId();
    }
    return 0;
}

void DeclarativeWebContainer::updateMode()
{
    setTabModel((BrowserAppInfo::captivePortal() || m_privateMode) ? m_privateTabModel.data()
                                                                   : m_persistentTabModel.data());
    emit tabIdChanged();

    // Reload active tab from new mode
    if (m_model->count() > 0) {
        reload(false);
    } else {
        setWebPage(nullptr);
        emit contentItemChanged();
    }
}

/**
 * @brief DeclarativeWebContainer::setActiveTabRendered
 * Sets the active tab render state. Should be only called when tab changes
 * or is about to change. When a frame is rendered, we mark tab as rendered.
 * @param rendered
 */
void DeclarativeWebContainer::setActiveTabRendered(bool rendered)
{
    QMutexLocker lock(&m_contextMutex);
    // When tab is closed, make sure that signal gets emitted again
    // if tab is already rendered. Value read from compositor thread. Thus,
    // guard with context mutex.
    m_activeTabRendered = rendered;
    emit activeTabRenderedChanged();
}

void DeclarativeWebContainer::clearWindowSurface()
{
    Q_ASSERT(m_context);
    // The GL context should always be used from the same thread in which it was created.
    Q_ASSERT(m_context->thread() == QThread::currentThread());
    m_context->makeCurrent(this);
    QOpenGLFunctions_ES2* funcs = m_context->versionFunctions<QOpenGLFunctions_ES2>();
    Q_ASSERT(funcs);

    setGLClearColor(funcs, m_webContentBackgroundColor);
    funcs->glClear(GL_COLOR_BUFFER_BIT);
    m_context->swapBuffers(this);
}

bool DeclarativeWebContainer::ensureRenderContext()
{
    if (m_context) {
        return m_context->makeCurrent(this);
    }

    QOpenGLContext *context = new QOpenGLContext;
    context->setFormat(requestedFormat());
    if (!context->create()) {
        qWarning() << "Failed to create browser WebRender presentation context";
        delete context;
        return false;
    }

    if (!context->makeCurrent(this)) {
        qWarning() << "Failed to make browser WebRender presentation context current";
        delete context;
        return false;
    }

    m_context = context;
    initializeOpenGLFunctions();
    return true;
}

bool DeclarativeWebContainer::ensureTextureProgram(QMozTextureTarget textureTarget)
{
    QOpenGLShaderProgram *&textureProgram = textureTarget == QMozTextureTarget::ExternalOES
            ? m_externalTextureProgram : m_texture2DProgram;
    if (textureProgram) {
        return true;
    }

    QOpenGLShaderProgram *program = new QOpenGLShaderProgram;
    static const char *vertexShader =
            "attribute highp vec2 aVertex;\n"
            "attribute highp vec2 aTexCoord;\n"
            "varying highp vec2 vTexCoord;\n"
            "void main() {\n"
            "    gl_Position = vec4(aVertex, 0.0, 1.0);\n"
            "    vTexCoord = aTexCoord;\n"
            "}\n";
    static const char *texture2DFragmentShader =
            "uniform lowp sampler2D texture;\n"
            "varying highp vec2 vTexCoord;\n"
            "void main() {\n"
            "    gl_FragColor = texture2D(texture, vTexCoord);\n"
            "}\n";
    static const char *externalTextureFragmentShader =
            "#extension GL_OES_EGL_image_external : require\n"
            "uniform lowp samplerExternalOES texture;\n"
            "varying highp vec2 vTexCoord;\n"
            "void main() {\n"
            "    gl_FragColor = texture2D(texture, vTexCoord);\n"
            "}\n";
    const char *fragmentShader = textureTarget == QMozTextureTarget::ExternalOES
            ? externalTextureFragmentShader : texture2DFragmentShader;

    if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader)
            || !program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader)
            || !program->link()) {
        qWarning() << "Failed to build browser WebRender texture shader"
                   << program->log();
        delete program;
        return false;
    }

    textureProgram = program;
    return true;
}

bool DeclarativeWebContainer::bindWebRenderFrameTexture(QSize *textureSize,
                                                         QMozTextureTarget *textureTarget)
{
    static const PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
            reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
                eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!glEGLImageTargetTexture2DOES) {
        qWarning() << "glEGLImageTargetTexture2DOES unavailable";
        return false;
    }

    bool hasImage = false;

    const bool delivered = m_mozWindow->withPlatformImage([&](const QMozEGLImage &image) {
        if (!image.image || image.size.isEmpty()) {
            return;
        }

        if (m_frameTexture != 0 && m_frameTextureTarget != image.textureTarget) {
            glDeleteTextures(1, &m_frameTexture);
            m_frameTexture = 0;
        }
        m_frameTextureTarget = image.textureTarget;

        if (m_frameTexture == 0) {
            glGenTextures(1, &m_frameTexture);
        }

        const GLenum target = glTextureTarget(m_frameTextureTarget);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(target, m_frameTexture);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glEGLImageTargetTexture2DOES(target, static_cast<GLeglImageOES>(image.image));

        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            qWarning() << "Failed to bind browser WebRender EGLImage"
                       << QString::number(error, 16)
                       << "image" << image.image << "size" << image.size;
            return;
        }

        hasImage = true;
        if (textureSize) {
            *textureSize = image.size;
        }
        if (textureTarget) {
            *textureTarget = m_frameTextureTarget;
        }
    });

    return delivered && hasImage && m_frameTexture != 0;
}

bool DeclarativeWebContainer::drawWebRenderFrame(const QRectF &targetRect,
                                                 const QSizeF &surfaceSize,
                                                 Qt::ScreenOrientation orientation,
                                                 QMozTextureTarget textureTarget,
                                                 const QRectF &textureRect)
{
    if (surfaceSize.width() <= 0.0 || surfaceSize.height() <= 0.0) {
        return false;
    }

    const GLfloat left = 2.0f * targetRect.left() / surfaceSize.width() - 1.0f;
    const GLfloat right = 2.0f * targetRect.right() / surfaceSize.width() - 1.0f;
    const GLfloat top = 1.0f - 2.0f * targetRect.top() / surfaceSize.height();
    const GLfloat bottom = 1.0f - 2.0f * targetRect.bottom() / surfaceSize.height();
    const GLfloat vertices[] = {
        left, top,
        left, bottom,
        right, top,
        right, bottom
    };
    GLfloat texCoords[8];
    updateTextureCoordinates(orientation, texCoords);
    applyTextureRect(textureRect, texCoords);

    QOpenGLShaderProgram *textureProgram = textureTarget == QMozTextureTarget::ExternalOES
            ? m_externalTextureProgram : m_texture2DProgram;
    if (!textureProgram || m_frameTexture == 0 || m_frameTextureTarget != textureTarget) {
        return false;
    }

    textureProgram->bind();
    textureProgram->setUniformValue("texture", 0);
    const int vertexAttribute = textureProgram->attributeLocation("aVertex");
    const int texCoordAttribute = textureProgram->attributeLocation("aTexCoord");
    textureProgram->enableAttributeArray(vertexAttribute);
    textureProgram->enableAttributeArray(texCoordAttribute);
    textureProgram->setAttributeArray(vertexAttribute, GL_FLOAT, vertices, 2);
    textureProgram->setAttributeArray(texCoordAttribute, GL_FLOAT, texCoords, 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(glTextureTarget(textureTarget), m_frameTexture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    textureProgram->disableAttributeArray(vertexAttribute);
    textureProgram->disableAttributeArray(texCoordAttribute);
    textureProgram->release();

    return glGetError() == GL_NO_ERROR;
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
    if (obj == m_chromeWindow) {
        if (event->type() == QEvent::Close) {
            m_closeEventFilter->applicationClosingStarted();
            if (!m_closing) {
                m_webPages->clear();
                bool initialUrl = hasInitialUrl();
                m_initialUrl.clear();
                m_fromExternal = false;
                if (initialUrl) {
                    emit hasInitialUrlChanged();
                }

                m_initialized = false;
                destroyWindow();
                if (QMozContext::instance()->getNumberOfWindows() != 0) {
                    m_closing = true;
                } else {
                    m_closeEventFilter->closeApplication();
                }
            }
            emit applicationClosing();
        } else if (event->type() == QEvent::Show) {
            if (!handle()) {
                m_closeEventFilter->cancelCloseApplication();
                create();
                show();
            }
        }
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

void DeclarativeWebContainer::destroyWindow()
{
    if (QMozContext::instance()->getNumberOfViews() != 0) {
        return;
    }

    if (m_mozWindow) {
        if (m_mozWindow->isReserved()) {
            connect(m_mozWindow.data(), &QMozWindow::released,
                    m_mozWindow.data(), &QObject::deleteLater);
            m_mozWindow->release();
        } else {
            delete m_mozWindow;
        }
        m_mozWindow = nullptr;
    }
}

bool DeclarativeWebContainer::event(QEvent *event)
{
    if (QPlatformWindow *windowHandle = event->type() == QEvent::PlatformSurface
                && static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated
            ? handle()
            : nullptr) {
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        native->setWindowProperty(windowHandle, QStringLiteral("BACKGROUND_VISIBLE"), false);
        native->setWindowProperty(windowHandle, QStringLiteral("HAS_CHILD_WINDOWS"), true);
    }
    return QWindow::event(event);
}

DeclarativeTabModel *DeclarativeWebContainer::privateTabModel() const
{
    return m_privateTabModel;
}

DeclarativeTabModel *DeclarativeWebContainer::persistentTabModel() const
{
    return m_persistentTabModel;
}

void DeclarativeWebContainer::exposeEvent(QExposeEvent*)
{
    // Filter out extra expose event spam. We often get 3-4 expose events
    // in a row. For all of them isExposed returns true. We only want to
    // clear the compositing surface once in such case.
    static bool alreadyExposed = false;

    if (isExposed() && !alreadyExposed) {
        initialize();

        if (m_chromeWindow) {
            m_chromeWindow->update();
        }

        if (m_webPage && !m_closing) {
            m_webPage->update();
        } else {
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

    if (isExposed() && m_frameTexture) {
        renderCompositedFrame();
    }
}

void DeclarativeWebContainer::resizeEvent(QResizeEvent *event)
{
    if (!event->size().isEmpty()) {
        updateMozWindowSize();
    }
    if (isExposed() && m_frameTexture) {
        renderCompositedFrame();
    }
    QWindow::resizeEvent(event);
}

void DeclarativeWebContainer::touchEvent(QTouchEvent *event)
{
    if (!m_rotationHandler) {
        qWarning() << "Cannot deliver touch events without rotationHandler";
        return;
    }

    if (m_webPage && m_enabled && (!m_touchBlocked || event->type() != QEvent::TouchBegin)) {
        QList<QTouchEvent::TouchPoint> touchPoints = event->touchPoints();
        QTouchEvent mappedTouchEvent = *event;
        const QPointF windowOffset(position());
        const QRectF contentRect = effectiveWebContentRect();
        const QPointF topLeft = m_rotationHandler->mapFromScene(windowOffset + contentRect.topLeft());
        const QPointF topRight = m_rotationHandler->mapFromScene(windowOffset + contentRect.topRight());
        const QPointF bottomLeft = m_rotationHandler->mapFromScene(windowOffset + contentRect.bottomLeft());
        const QPointF bottomRight = m_rotationHandler->mapFromScene(windowOffset + contentRect.bottomRight());
        const QPointF contentOrigin(qMin(qMin(topLeft.x(), topRight.x()), qMin(bottomLeft.x(), bottomRight.x())),
                                    qMin(qMin(topLeft.y(), topRight.y()), qMin(bottomLeft.y(), bottomRight.y())));

        for (int i = 0; i < touchPoints.count(); ++i) {
            QPointF pt = m_rotationHandler->mapFromScene(touchPoints.at(i).pos() + windowOffset);
            pt -= contentOrigin;
            touchPoints[i].setPos(pt);
        }

        mappedTouchEvent.setTouchPoints(touchPoints);
        m_webPage->touchEvent(&mappedTouchEvent);
        if (event->type() == QEvent::TouchBegin) {
            emit touched();
        }
    } else {
        QWindow::touchEvent(event);
    }
}

void DeclarativeWebContainer::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::BackButton:
        event->accept();
        emit backButtonPressed();
        break;
    case Qt::ForwardButton:
        event->accept();
        emit forwardButtonPressed();
        break;
    default:
        QWindow::mousePressEvent(event);
    }
}

void DeclarativeWebContainer::wheelEvent(QWheelEvent *event)
{
    if (m_webPage && m_enabled) {
        m_webPage->wheelEvent(event);
    }
}

void DeclarativeWebContainer::keyPressEvent(QKeyEvent *event)
{
    emit keyPressed(event->key());

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
    if (gForceLandscapeToPortrait) {
        if (orientation == Qt::LandscapeOrientation) {
            orientation = Qt::PortraitOrientation;
        } else if (orientation == Qt::InvertedLandscapeOrientation) {
            orientation = Qt::InvertedPortraitOrientation;
        }
    }

    if (m_mozWindow) {
        bool orientationShouldChange = (orientation != m_mozWindow->pendingOrientation());
        m_mozWindow->setContentOrientation(orientation);
        if (orientationShouldChange) {
            emit pendingWebContentOrientationChanged();
        }
    }
    reportContentOrientationChange(orientation);
}

void DeclarativeWebContainer::clearSurface()
{
    if (!isExposed()) {
        return;
    }

    QMutexLocker lock(&m_contextMutex);
    if (m_context) {
        clearWindowSurface();
        return;
    }

    QOpenGLContext context;
    context.setFormat(requestedFormat());
    if (!context.create()) {
        qWarning() << "Failed to create browser surface clear context";
        return;
    }

    m_context = &context;
    clearWindowSurface();
    m_context = nullptr;

    context.doneCurrent();
}

qreal DeclarativeWebContainer::contentHeight() const
{
    if (m_webPage) {
        return m_webPage->contentHeight();
    } else {
        return 0.0;
    }
}

QRectF DeclarativeWebContainer::effectiveWebContentRect() const
{
    const QRectF windowRect(QPointF(0, 0), QSizeF(width(), height()));
    QRectF rect = m_webContentRect.isNull() ? windowRect : m_webContentRect;

    rect = rect.intersected(windowRect);
    if (rect.width() <= 0 || rect.height() <= 0) {
        return windowRect;
    }

    return rect;
}

QSize DeclarativeWebContainer::webContentSize() const
{
    const QRectF rect = effectiveWebContentRect();
    return QSize(qMax(1, qRound(rect.width())),
                 qMax(1, qRound(rect.height())));
}

void DeclarativeWebContainer::updateMozWindowSize()
{
    if (m_mozWindow) {
        m_mozWindow->setSize(webContentSize());
    }
}

void DeclarativeWebContainer::onActiveTabChanged(int activeTabId)
{
    if (activeTabId <= 0) {
        return;
    }

    reload(false);

    if (m_model->activeTab().hidden()) {
        restorePreviousTab();
    }
}

void DeclarativeWebContainer::restorePreviousTab()
{
    // Switch back to the old tab
    if (m_PreviousTabWhenHidden >= 0) {
        m_hiddenTabTimer.start();
    }
}

void DeclarativeWebContainer::restorePreviousTabDelayed()
{
    // Called when a hidden tab has been opened
    if (m_PreviousTabWhenHidden >= 0 && m_model) {
        // Restore the previous tab to hide the hidden tab
        m_model->activateTabById(m_PreviousTabWhenHidden);
        m_PreviousTabWhenHidden = -1;
    }
}

void DeclarativeWebContainer::initialize()
{
    if (!isExposed() || m_closing) {
        return;
    }

    if (SailfishOS::WebEngine::instance()->isInitialized() && !m_mozWindow) {
        m_mozWindow = new QMozWindow(webContentSize());
        m_mozWindow->setPrimaryOrientation(screen()->primaryOrientation());

        connect(m_mozWindow.data(), &QMozWindow::orientationChangeFiltered,
                this, &DeclarativeWebContainer::handleContentOrientationChanged);
        connect(m_mozWindow.data(), &QMozWindow::compositingFinished,
                this, &DeclarativeWebContainer::handleCompositingFinished, Qt::QueuedConnection);
        m_mozWindow->reserve();
        m_mozWindow->setReadyToPaint(false);
        if (m_chromeWindow) {
            updateContentOrientation(m_chromeWindow->contentOrientation());
        }
    }

    // This signal handler is responsible for activating
    // the first page.
    if (!canInitialize() || m_initialized) {
        return;
    }

    // Page activation needs this QML component. It can arrive after the
    // engine/model/expose signals that normally trigger initialization.
    if (!m_webPageComponent) {
        return;
    }

    // From this point onwards, we're ready to initialize.
    // We set m_initialized to true prior to the block below since we may need to
    // call loadTab() within it, and that function is guarded by the value of m_initialized.
    m_initialized = true;

    // Load test
    // 1) no tabs and firstUseDone or we have incoming url, try to active tab, only after that fails
    //    load initial url or home page to a new tab.
    // 2) model has tabs, load initial url or active tab.
    bool firstUseDone = DeclarativeWebUtils::instance()->firstUseDone();
    if ((m_model->count() == 0 && firstUseDone) || !m_initialUrl.isEmpty()) {
        QString url = m_initialUrl;
        if (m_initialUrl.isEmpty()) {
            if (!browserEnabled()) {
                url = ABOUT_BLANK;
            } else {
                url = DeclarativeWebUtils::instance()->homePage();
            }
        }

        if (!m_model->activateTab(url, true)) {
            m_model->newTab(url, true);
        }
    } else if (m_model->count() > 0 && !m_webPage) {
        Tab tab = m_model->activeTab();
        if (!m_initialUrl.isEmpty()) {
            tab.setRequestedUrl(m_initialUrl);
        }
        loadTab(tab, true, m_fromExternal);
    }

    if (!m_completed) {
        m_completed = true;
        emit completedChanged();
    }

    bool initialUrl = hasInitialUrl();
    m_initialUrl.clear();
    m_fromExternal = false;

    if (initialUrl) {
        emit hasInitialUrlChanged();
    }
}

void DeclarativeWebContainer::onDownloadStarted()
{
    emit m_webPage->urlChanged();

    if (m_model->count() == 0) {
        // Download doesn't add tab to model. Mimic
        // model change in case downloading was started without
        // existing tabs.
        emit m_model->countChanged();
    }
}

void DeclarativeWebContainer::onNewTabRequested(const Tab &tab, bool fromExternal)
{
    if (tab.hidden()) {
        m_PreviousTabWhenHidden = m_webPage->tabId();
    }

    if (activatePage(tab, false, fromExternal)) {
        m_webPage->loadTab(tab.requestedUrl(), false, fromExternal);
    }
}

void DeclarativeWebContainer::releasePage(int tabId)
{
    m_tabOwners.remove(tabId);
    if (m_webPages) {
        m_webPages->release(tabId);
        // Successfully destroyed. Emit relevant property changes.
        if (m_model->count() == 0) {
            setWebPage(nullptr, true);
        }
    }
}

void DeclarativeWebContainer::closeWindow()
{
    DeclarativeWebPage *webPage = qobject_cast<DeclarativeWebPage *>(sender());
    // Closing only allowed if window was created by script i.e. has parent.
    if (webPage && webPage->parentId() > 0 && m_model) {
        int parentPageTabId = tabId(webPage->parentId());
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
        if (m_waitingForActiveTabFrame) {
            m_waitingForActiveTabLoad = true;
            m_waitingForActiveTabFirstPaint = true;
            m_activeTabCompositesToSkip = 0;
            clearSurface();
        }
    }

    emit loadingChanged();
}

void DeclarativeWebContainer::handleActiveTabFirstPaint(int offx, int offy)
{
    if (sender() == m_webPage && offx >= 0 && offy >= 0) {
        // Some pages keep the top-level load open after useful content is
        // already painted. First paint is enough to release the cleared tab
        // surface; keep skipping one composite to avoid stale buffer reuse.
        m_waitingForActiveTabLoad = false;
        m_waitingForActiveTabFirstPaint = false;
        m_activeTabCompositesToSkip = qMax(m_activeTabCompositesToSkip, 1);
    }

    updateActiveTabRendered();
}

void DeclarativeWebContainer::updateActiveTabRendered()
{
    if (m_activeTabRendered || m_waitingForActiveTabFrame || !m_webPage || !m_webPage->completed()
            || (!m_webPage->domContentLoaded() && !m_webPage->isPainted())) {
        return;
    }

    setActiveTabRendered(true);
}

void DeclarativeWebContainer::onLastViewDestroyed()
{
    if (m_closing) {
        destroyWindow();
    }
}

void DeclarativeWebContainer::onLastWindowDestroyed()
{
    m_closing = false;

    if (isExposed()) {
        initialize();
    }
    if (!handle()) {
        m_closeEventFilter->closeApplication();
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

bool DeclarativeWebContainer::canInitialize() const
{
    return SailfishOS::WebEngine::instance()->isInitialized() && m_model && m_model->loaded();
}

bool DeclarativeWebContainer::browserEnabled() const
{
    return Sailfish::PolicyValue::keyValue(Sailfish::PolicyValue::BrowserEnabled).toBool();
}

void DeclarativeWebContainer::loadTab(const Tab& tab, bool force, bool fromExternal)
{
    if (activatePage(tab, true, fromExternal) || force) {
        // Note: active pages containing a "link" between each other (parent-child relationship)
        // are not destroyed automatically e.g. in low memory notification.
        // Hence, parentId is not necessary over here.
        m_webPage->loadTab(tab.url(), force, fromExternal);
    }
}

void DeclarativeWebContainer::handleCompositingFinished()
{
    renderCompositedFrame();
    updateActiveTabRendered();
}

void DeclarativeWebContainer::renderCompositedFrame()
{
    if (!isExposed() || !m_mozWindow) {
        return;
    }

    if (m_waitingForActiveTabFrame) {
        // Do not redraw the previous tab's cached WebRender image after a tab
        // switch or before a switched-to loading tab has produced real paint.
        if (m_webPage && m_waitingForActiveTabLoad && m_webPage->loaded()) {
            m_waitingForActiveTabLoad = false;
            m_activeTabCompositesToSkip = qMax(m_activeTabCompositesToSkip, 1);
        }

        if (!m_webPage || m_waitingForActiveTabFirstPaint) {
            clearSurface();
            return;
        }
        if (m_activeTabCompositesToSkip > 0) {
            --m_activeTabCompositesToSkip;
            clearSurface();
            return;
        }
    }

    if (!m_webPage) {
        return;
    }

    const bool completingActiveTabFrame = m_waitingForActiveTabFrame;

    if (!ensureRenderContext()) {
        return;
    }

    QSize textureSize;
    QMozTextureTarget textureTarget;
    if (!bindWebRenderFrameTexture(&textureSize, &textureTarget)
            || !ensureTextureProgram(textureTarget)) {
        static int noImageWarnings = 0;
        if (++noImageWarnings <= 5) {
            qWarning() << "No WebRender EGLImage available for browser frame";
        }
        return;
    }

    const qreal ratio = devicePixelRatio();
    glViewport(0, 0, qRound(width() * ratio), qRound(height() * ratio));
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    const QColor clearColor = opaqueSurfaceColor(m_webContentBackgroundColor);
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!drawWebRenderFrame(effectiveWebContentRect(), QSizeF(width(), height()),
                            m_mozWindow->contentOrientation(), textureTarget)) {
        qWarning() << "Browser WebRender frame draw failed"
                   << "textureSize" << textureSize << "windowSize" << size();
        return;
    }

    m_context->swapBuffers(this);

    if (completingActiveTabFrame) {
        m_waitingForActiveTabFrame = false;
        m_waitingForActiveTabLoad = false;
        m_waitingForActiveTabFirstPaint = false;
    }
}

void DeclarativeWebContainer::handleContentOrientationChanged(Qt::ScreenOrientation orientation)
{
    if (orientation == pendingWebContentOrientation()) {
        emit webContentOrientationChanged(orientation);
        renderCompositedFrame();
    }
}

DeclarativeHistoryModel *DeclarativeWebContainer::historyModel() const
{
    return m_historyModel;
}

void DeclarativeWebContainer::setHistoryModel(DeclarativeHistoryModel *model)
{
    if (model != m_historyModel) {
        m_historyModel = model;
        emit historyModelChanged();
    }
}

bool DeclarativeWebContainer::hasInitialUrl() const
{
    return !m_initialUrl.isEmpty();
}

void DeclarativeWebContainer::dsmeStateChange(const QString &state)
{
    if ((state == "REBOOT" || state == "SHUTDOWN") && m_closeEventFilter)
        m_closeEventFilter->closeApplication();
}
