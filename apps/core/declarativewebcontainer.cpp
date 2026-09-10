/*
 * Copyright (c) 2013 - 2021 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "declarativewebcontainer.h"
#include "persistenttabmodel.h"
#include "privatetabmodel.h"
#include "dbmanager.h"
#include "downloadmanager.h"
#include "browserappinfo.h"
#include "logging.h"
#include "closeeventfilter.h"
#include "declarativehistorymodel.h"
#include <webengine.h>
#include <QGuiApplication>
#include <QWindow>
#include <QScreen>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPlatformSurfaceEvent>
#include <qpa/qplatformnativeinterface.h>
#include <MDConfItem>
#include <QDBusConnection>
#include <dsme/dsme_dbus_if.h>
#include <libsailfishpolicy/policyvalue.h>

static DeclarativeWebContainer *s_instance = nullptr;

DeclarativeWebContainer::DeclarativeWebContainer(QQuickItem *parent)
    : QQuickItem(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *window) {
        setChromeWindow(window);
    });
    if (nativePresentationEnabled()) {
        m_nativeWindow = new QWindow;
        m_nativeWindow->setSurfaceType(QWindow::OpenGLSurface);
        QSurfaceFormat format;
        format.setRenderableType(QSurfaceFormat::OpenGLES);
        format.setVersion(2, 0);
        format.setRedBufferSize(5);
        format.setGreenBufferSize(6);
        format.setBlueBufferSize(5);
        format.setAlphaBufferSize(0);
        format.setDepthBufferSize(0);
        format.setStencilBufferSize(0);
        m_nativeWindow->setFormat(format);
        m_nativeWindow->resize(qApp->primaryScreen()->size());
        m_nativeWindow->setTitle(QStringLiteral("BrowserContent"));
        m_nativeWindow->setObjectName(QStringLiteral("WebView"));
    }
    MDConfItem privateAutostart(QStringLiteral("/apps/sailfish-browser/settings/browser_privatebrowsing_autostart"));
    m_privateMode = BrowserAppInfo::captivePortal() || !browserEnabled()
            || privateAutostart.value(false).toBool();
    const int maxTabId = DBManager::instance()->getMaxTabId();
    m_persistentTabModel = new PersistentTabModel(maxTabId + 1, this);
    m_privateTabModel = new PrivateTabModel(maxTabId + 1001, this);
    setTabModel(m_privateMode ? m_privateTabModel.data() : m_persistentTabModel.data());
    connect(SailfishOS::WebEngine::instance(), &SailfishOS::WebEngine::initialized,
            this, &DeclarativeWebContainer::initialize);
    m_closeEventFilter = new CloseEventFilter(DownloadManager::instance(), this);
    qApp->installEventFilter(this);
    QDBusConnection::systemBus().connect(dsme_service, dsme_sig_path, dsme_sig_interface,
                                        dsme_state_change_ind, this, "1dsmeStateChange(QString)");
}

DeclarativeWebContainer::~DeclarativeWebContainer()
{
    delete m_nativeWindow;
    s_instance = nullptr;
}

bool DeclarativeWebContainer::nativePresentationEnabled()
{
    // Select once per process so changing dconf cannot mix presentation paths.
    static const bool enabled = []() {
        MDConfItem setting(QStringLiteral("/apps/sailfish-browser/settings/native_presentation"));
        return setting.value(true).toBool();
    }();
    return enabled;
}

QWindow *DeclarativeWebContainer::nativeWindow() const
{
    return m_nativeWindow;
}

DeclarativeWebContainer *DeclarativeWebContainer::instance()
{
    Q_ASSERT(s_instance);
    return s_instance;
}

DeclarativeTabModel *DeclarativeWebContainer::tabModel() const
{
    return m_model;
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

QObject *DeclarativeWebContainer::chromeWindow() const
{
    return m_chromeWindow;
}

int DeclarativeWebContainer::tabId() const
{
    Q_ASSERT(!!m_model);
    return m_model->activeTabId();
}

void DeclarativeWebContainer::closeTab(int tabId)
{
    m_model->removeTabById(tabId, false);
}

void DeclarativeWebContainer::updateHostedState(const QString &url, const QString &title,
                                                 bool loading, int loadProgress,
                                                 bool canGoBack, bool canGoForward,
                                                 QMozSecurity *security,
                                                 bool notifySecurity)
{
    const bool stateWasActive = m_hostedStateActive;
    const bool urlDidChange = stateWasActive ? m_hostedUrl != url : this->url() != url;
    const bool titleDidChange = stateWasActive ? m_hostedTitle != title : this->title() != title;
    const bool loadingDidChange = stateWasActive ? m_hostedLoading != loading
                                                  : this->loading() != loading;
    const bool progressDidChange = stateWasActive ? m_hostedLoadProgress != loadProgress
                                                   : this->loadProgress() != loadProgress;
    const bool canGoBackDidChange = stateWasActive ? m_hostedCanGoBack != canGoBack
                                                    : this->canGoBack() != canGoBack;
    const bool canGoForwardDidChange = stateWasActive ? m_hostedCanGoForward != canGoForward
                                                       : this->canGoForward() != canGoForward;
    const bool securityDidChange = notifySecurity
            || (stateWasActive ? m_hostedSecurity.data() != security
                               : this->security() != security);

    m_hostedUrl = url;
    m_hostedTitle = title;
    m_hostedLoading = loading;
    m_hostedLoadProgress = loadProgress;
    m_hostedCanGoBack = canGoBack;
    m_hostedCanGoForward = canGoForward;
    m_hostedStateActive = true;
    m_hostedSecurity = security;

    if (urlDidChange) {
        emit urlChanged();
    }
    if (titleDidChange) {
        emit titleChanged();
    }
    if (loadingDidChange) {
        emit loadingChanged();
    }
    if (progressDidChange) {
        emit loadProgressChanged();
    }
    if (canGoBackDidChange) {
        emit canGoBackChanged();
    }
    if (canGoForwardDidChange) {
        emit canGoForwardChanged();
    }
    if (securityDidChange) {
        emit securityChanged();
    }
}

void DeclarativeWebContainer::clearHostedState()
{
    const bool urlDidChange = !m_hostedUrl.isEmpty();
    const bool titleDidChange = !m_hostedTitle.isEmpty();
    const bool loadingDidChange = m_hostedLoading;
    const bool progressDidChange = m_hostedLoadProgress != 0;
    const bool canGoBackDidChange = m_hostedCanGoBack;
    const bool canGoForwardDidChange = m_hostedCanGoForward;
    const bool securityDidChange = !m_hostedSecurity.isNull();

    m_hostedUrl.clear();
    m_hostedTitle.clear();
    m_hostedLoading = false;
    m_hostedLoadProgress = 0;
    m_hostedCanGoBack = false;
    m_hostedCanGoForward = false;
    m_hostedStateActive = false;
    m_hostedSecurity.clear();

    if (urlDidChange) {
        emit urlChanged();
    }
    if (titleDidChange) {
        emit titleChanged();
    }
    if (loadingDidChange) {
        emit loadingChanged();
    }
    if (progressDidChange) {
        emit loadProgressChanged();
    }
    if (canGoBackDidChange) {
        emit canGoBackChanged();
    }
    if (canGoForwardDidChange) {
        emit canGoForwardChanged();
    }
    if (securityDidChange) {
        emit securityChanged();
    }
}

int DeclarativeWebContainer::activateTab(int tabId, const QString &url)
{
    return requestTabWithOwner(tabId, url, 0);
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

DeclarativeTabModel *DeclarativeWebContainer::privateTabModel() const
{
    return m_privateTabModel;
}

DeclarativeTabModel *DeclarativeWebContainer::persistentTabModel() const
{
    return m_persistentTabModel;
}

bool DeclarativeWebContainer::canInitialize() const
{
    return SailfishOS::WebEngine::instance()->isInitialized() && m_model && m_model->loaded();
}

bool DeclarativeWebContainer::browserEnabled() const
{
    return Sailfish::PolicyValue::keyValue(Sailfish::PolicyValue::BrowserEnabled).toBool();
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

int DeclarativeWebContainer::requestTabWithOwner(int tabId, const QString &url, uint ownerPid)
{
    if (m_model->contains(tabId)) {
        if (url.isEmpty()) {
            m_model->activateTabById(tabId);
        } else if (!m_model->requestRuntimeTabNavigation(tabId, url, false)) {
            qCWarning(lcCoreLog) << "Cannot navigate hosted tab" << tabId;
        }
    } else {
        tabId = m_model->newTab(url, false);
        if (ownerPid && tabId > 0) {
            m_tabOwners.insert(tabId, ownerPid);
        }
    }
    return tabId;
}

void DeclarativeWebContainer::setTabModel(DeclarativeTabModel *model)
{
    if (m_model == model) return;
    if (m_model) disconnect(m_model, nullptr, this, nullptr);
    m_model = model;
    connect(model, &DeclarativeTabModel::loadedChanged, this, &DeclarativeWebContainer::initialize);
    connect(model, &DeclarativeTabModel::tabClosed, this, [this](int id) { m_tabOwners.remove(id); });
    emit tabModelChanged();
}

void DeclarativeWebContainer::updateMode()
{
    m_initialized = false;
    clearHostedState();
    setTabModel(m_privateMode ? m_privateTabModel.data() : m_persistentTabModel.data());
    emit tabIdChanged();
    initialize();
}

void DeclarativeWebContainer::setChromeWindow(QObject *window)
{
    QQuickView *view = qobject_cast<QQuickView *>(window);
    if (view && view != m_chromeWindow) {
        m_chromeWindow = view;
        if (m_nativeWindow) {
            m_chromeWindow->setTransientParent(m_nativeWindow);
            m_nativeWindow->showFullScreen();
        }
        m_chromeWindow->showFullScreen();
        emit chromeWindowChanged();
        initialize();
    }
}

void DeclarativeWebContainer::componentComplete()
{
    QQuickItem::componentComplete();
    setChromeWindow(window());
    initialize();
}

void DeclarativeWebContainer::initialize()
{
    if (m_closing || !isComponentComplete() || !canInitialize()) return;
    m_initialized = true;
    if (!m_initialUrl.isEmpty() && !m_model->activateTab(m_initialUrl, true)) {
        m_model->newTab(m_initialUrl, m_fromExternal);
    }
    if (!m_completed) {
        m_completed = true;
        emit completedChanged();
    }
    const bool initialUrl = hasInitialUrl();
    m_initialUrl.clear();
    m_fromExternal = false;
    if (initialUrl) emit hasInitialUrlChanged();
}

void DeclarativeWebContainer::load(const QString &url, bool, bool fromExternal)
{
    const QString requestedUrl = url.isEmpty() || !browserEnabled() ? QStringLiteral("about:blank") : url;
    if (!m_initialized || !canInitialize()) {
        m_initialUrl = requestedUrl;
        m_fromExternal = fromExternal;
    } else {
        emit hostedLoadRequested(requestedUrl, fromExternal);
    }
}

void DeclarativeWebContainer::reload(bool) { emit hostedReloadRequested(); }
void DeclarativeWebContainer::goBack() { emit hostedGoBackRequested(); }
void DeclarativeWebContainer::goForward() { emit hostedGoForwardRequested(); }
bool DeclarativeWebContainer::loading() const { return m_hostedLoading; }
int DeclarativeWebContainer::loadProgress() const { return m_hostedLoadProgress; }
bool DeclarativeWebContainer::canGoBack() const { return m_hostedCanGoBack; }
bool DeclarativeWebContainer::canGoForward() const { return m_hostedCanGoForward; }
QString DeclarativeWebContainer::url() const { return m_hostedUrl; }
QString DeclarativeWebContainer::title() const { return m_hostedTitle; }
QMozSecurity *DeclarativeWebContainer::security() const { return m_hostedSecurity; }
bool DeclarativeWebContainer::isActiveTab(int id) { return m_model && m_model->activeTabId() == id; }

bool DeclarativeWebContainer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_nativeWindow && event->type() == QEvent::Expose
            && m_nativeWindow->isExposed() && !m_nativeInitialized) {
        // Wayland cannot map a surface (or expose its transient QML window)
        // before the first buffer is committed. Gecko's view initialization
        // itself needs the QML window to render, so bootstrap independently.
        QOpenGLContext context;
        context.setFormat(m_nativeWindow->format());
        if (context.create() && context.makeCurrent(m_nativeWindow)) {
            context.functions()->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            context.functions()->glClear(GL_COLOR_BUFFER_BIT);
            context.swapBuffers(m_nativeWindow);
            context.doneCurrent();
            m_nativeInitialized = true;
        } else {
            qCWarning(lcCoreLog) << "Cannot initialize the native Browser surface";
        }
    }
    if (obj == m_nativeWindow && event->type() == QEvent::PlatformSurface) {
        auto *surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);
        m_nativeInitialized = false;
        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated
                && m_nativeWindow->handle()) {
            QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
            native->setWindowProperty(m_nativeWindow->handle(), QStringLiteral("BACKGROUND_VISIBLE"), false);
            native->setWindowProperty(m_nativeWindow->handle(), QStringLiteral("HAS_CHILD_WINDOWS"), true);
        }
    }
    if ((obj == m_chromeWindow || obj == m_nativeWindow) && event->type() == QEvent::Close && !m_closing) {
        m_closing = true;
        m_closeEventFilter->applicationClosingStarted();
        emit applicationClosing();
        m_closeEventFilter->closeApplication();
    }
    return QQuickItem::eventFilter(obj, event);
}

void DeclarativeWebContainer::reportWindowOrientation(Qt::ScreenOrientation orientation)
{
    if (m_chromeWindow) m_chromeWindow->reportContentOrientationChange(orientation);
}
