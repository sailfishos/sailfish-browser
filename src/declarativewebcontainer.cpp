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
#include "declarativetab.h"
#include "declarativetabmodel.h"
#include "dbmanager.h"

#include <QPointer>
#include <QTimerEvent>
#include <QQuickWindow>
#include <quickmozview.h>
#include <QDir>
#include <QTransform>
#include <QStandardPaths>
#include <QtConcurrentRun>
#include <QGuiApplication>
#include <QScreen>

#include <QMetaMethod>

DeclarativeWebContainer::DeclarativeWebContainer(QQuickItem *parent)
    : QQuickItem(parent)
    , m_webView(0)
    , m_model(0)
    , m_currentTab(new DeclarativeTab(this))
    , m_foreground(true)
    , m_background(false)
    , m_windowVisible(false)
    , m_backgroundTimer(0)
    , m_active(false)
    , m_popupActive(false)
    , m_portrait(true)
    , m_fullScreenMode(false)
    , m_inputPanelVisible(false)
    , m_inputPanelHeight(0.0)
    , m_inputPanelOpenHeight(0.0)
    , m_toolbarHeight(0.0)
    , m_canGoForward(false)
    , m_canGoBack(false)
    , m_realNavigation(false)
    , m_firstFrameRendered(false)
{
    setFlag(QQuickItem::ItemHasContents, true);
    connect(m_currentTab, SIGNAL(titleChanged()), this, SIGNAL(titleChanged()));
    connect(m_currentTab, SIGNAL(urlChanged()), this, SIGNAL(urlChanged()));
    connect(m_currentTab, SIGNAL(urlChanged()), this, SLOT(triggerLoad()));
    if (!window()) {
        connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
    } else {
        connect(window(), SIGNAL(visibleChanged(bool)), this, SLOT(windowVisibleChanged(bool)));
    }

    connect(&m_screenCapturer, SIGNAL(finished()), this, SLOT(screenCaptureReady()));

    QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheLocation);
    if(!dir.exists() && !dir.mkpath(cacheLocation)) {
        qWarning() << "Can't create directory "+ cacheLocation;
        return;
    }
}

DeclarativeWebContainer::~DeclarativeWebContainer()
{
    // Disconnect all signal slot connections
    if (m_webView) {
        disconnect(m_webView, 0, 0, 0);
    }

    m_screenCapturer.cancel();
    m_screenCapturer.waitForFinished();
}

QuickMozView *DeclarativeWebContainer::webView() const
{
    return m_webView;
}

void DeclarativeWebContainer::setWebView(QuickMozView *webView)
{
    if (m_webView != webView) {
        if (m_webView) {
            disconnect(m_webView);
        }

        if (webView) {
            connect(webView, SIGNAL(imeNotification(int,bool,int,int,QString)),
                    this, SLOT(imeNotificationChanged(int,bool,int,int,QString)));
            connect(webView, SIGNAL(contentHeightChanged()), this, SLOT(resetHeight()));
            connect(webView, SIGNAL(scrollableOffsetChanged()), this, SLOT(resetHeight()));
            connect(this, SIGNAL(heightChanged()), this, SLOT(resetHeight()));
        }
        m_webView = webView;
        emit contentItemChanged();
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

bool DeclarativeWebContainer::background() const
{
    return m_background;
}

bool DeclarativeWebContainer::active() const
{
    return m_active;
}

void DeclarativeWebContainer::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;
        emit activeChanged();

        // If dialog has been opened, we need to verify that input panel is not visible.
        // This might happen when the user fills in login details to a form and
        // presses enter to accept the form after which PasswordManagerDialog is pushed to pagestack
        // on top the BrowserPage. Once PassowordManagerDialog is accepted/rejected
        // this condition can be met. If active changes to true before keyboard is fully closed,
        // then the inputPanelVisibleChanged() signal is emitted by setInputPanelHeight.
        if (m_active && m_inputPanelHeight == 0 && m_inputPanelVisible) {
            m_inputPanelVisible = false;
            emit inputPanelVisibleChanged();
        }
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
        if (m_active) {
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

QString DeclarativeWebContainer::title() const
{
    return m_currentTab->title();
}

QString DeclarativeWebContainer::url() const
{
    return m_currentTab->url();
}

DeclarativeTab *DeclarativeWebContainer::currentTab() const
{
    return m_currentTab;
}

void DeclarativeWebContainer::goForward()
{
    if (m_canGoForward && m_currentTab->valid()) {
        m_currentTab->activateNextLink();
        if (m_webView && m_webView->canGoForward()) {
            m_realNavigation = true;
            m_webView->goForward();
        } else {
            m_realNavigation = false;
        }

        m_model->setBackForwardNavigation(true);
        DBManager::instance()->goForward(m_model->currentTabId());
    }
}

void DeclarativeWebContainer::goBack()
{
    if (m_canGoBack && m_currentTab->valid()) {
        m_currentTab->activatePreviousLink();
        if (m_webView && m_webView->canGoBack()) {
            m_realNavigation = true;
            m_webView->goBack();
        } else {
            m_realNavigation = false;
        }

        m_model->setBackForwardNavigation(true);
        DBManager::instance()->goBack(m_model->currentTabId());
    }
}

void DeclarativeWebContainer::captureScreen()
{
    if (!m_webView) {
        return;
    }

    if (m_active && m_firstFrameRendered && !m_popupActive) {
        int size = QGuiApplication::primaryScreen()->size().width();
        if (!m_portrait && !m_fullScreenMode) {
            size -= m_toolbarHeight;
        }

        qreal rotation = parentItem() ? parentItem()->rotation() : 0;
        captureScreen(url(), size, rotation);
    }
}

void DeclarativeWebContainer::resetHeight(bool respectContentHeight)
{
    if (!m_webView || !m_webView->state().isEmpty()) {
        return;
    }

    qreal fullHeight = height();

    // Application active
    if (respectContentHeight) {
        // Handle webView height over here, BrowserPage.qml loading
        // reset might be redundant as we have also loaded trigger
        // reset. However, I'd leave it there for safety reasons.
        // We need to reset height always back to short height when loading starts
        // so that after tab change there is always initial short composited height.
        // Height may expand when content is moved.
        if (contentHeight() > fullHeight + m_toolbarHeight) {
            m_webView->setHeight(fullHeight);
        } else {
            m_webView->setHeight(fullHeight - m_toolbarHeight);
        }
    } else {
        m_webView->setHeight(fullHeight - m_toolbarHeight);
    }
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
    if (m_webView) {
        return m_webView->contentHeight();
    } else {
        return 0.0;
    }
}

DeclarativeWebContainer::ScreenCapture DeclarativeWebContainer::saveToFile(QString url, QImage image, QRect cropBounds, int tabId, qreal rotate)
{
    QString path = QString("%1/tab-%2-thumb.png").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).arg(tabId);
    QTransform transform;
    transform.rotate(360 - rotate);
    image = image.transformed(transform);
    image = image.copy(cropBounds);

    ScreenCapture capture;
    if(image.save(path)) {
        capture.tabId = tabId;
        capture.path = path;
        capture.url = url;
    } else {
        capture.tabId = -1;
        qWarning() << Q_FUNC_INFO << "failed to save image" << path;
    }
    return capture;
}

void DeclarativeWebContainer::timerEvent(QTimerEvent *event)
{
    if (m_backgroundTimer == event->timerId()) {
        if (window()) {
            // Guard window visibility change was not cancelled after timer triggered.
            bool tmpVisible = window()->isVisible();
            // m_windowVisible == m_background visibility changed
            if (tmpVisible == m_windowVisible && m_windowVisible == m_background) {
                m_background = !m_windowVisible;
                emit backgroundChanged();
            }
        }
        killTimer(m_backgroundTimer);
    }
}

void DeclarativeWebContainer::componentComplete()
{
    QQuickItem::componentComplete();
    QVariant var = property("tabModel");
    m_model = qobject_cast<DeclarativeTabModel *>(qvariant_cast<QObject*>(var));
    m_model->setCurrentTab(m_currentTab);
    connect(m_model, SIGNAL(_activeTabChanged(const Tab&)), this, SLOT(updateTabData(const Tab&)));
    connect(m_model, SIGNAL(_activeTabInvalidated()), this, SLOT(invalidateTabData()));
}


void DeclarativeWebContainer::windowVisibleChanged(bool visible)
{
    Q_UNUSED(visible);
    if (window()) {
        m_windowVisible = window()->isVisible();
        m_backgroundTimer = startTimer(1000);
    }
}

void DeclarativeWebContainer::handleWindowChanged(QQuickWindow *window)
{
    if (window) {
        connect(window, SIGNAL(visibleChanged(bool)), this, SLOT(windowVisibleChanged(bool)));
    }
}

void DeclarativeWebContainer::updateTabData(const Tab &tab)
{
    m_currentTab->updateTabData(tab);

#ifdef DEBUG_LOGS
    qDebug() << "canGoBack = " << m_canGoBack << "canGoForward = " << m_canGoForward << &tab;
#endif
    if (m_canGoForward != (tab.nextLink() > 0)) {
        m_canGoForward = tab.nextLink() > 0;
        emit canGoForwardChanged();
    }

    if (m_canGoBack != (tab.previousLink() > 0)) {
        m_canGoBack = tab.previousLink() > 0;
        emit canGoBackChanged();
    }
}

void DeclarativeWebContainer::invalidateTabData()
{
    m_currentTab->invalidateTabData();
    if (m_canGoForward) {
        m_canGoForward = false;
        emit canGoForwardChanged();
    }

    if (m_canGoBack) {
        m_canGoBack = false;
        emit canGoBack();
    }
}

void DeclarativeWebContainer::screenCaptureReady()
{
    ScreenCapture capture = m_screenCapturer.result();
#ifdef DEBUG_LOGS
    qDebug() << capture.tabId << capture.path << capture.url;
#endif
    if (capture.tabId != -1) {
        // Update immediately without dbworker round trip.
        if (capture.tabId == m_currentTab->tabId()) {
            m_currentTab->setThumbnailPath(capture.path);
        }
        // TODO: Cleanup url.
        DBManager::instance()->updateThumbPath(capture.url, capture.path, capture.tabId);
    }
}

void DeclarativeWebContainer::triggerLoad()
{
    bool realNavigation = m_realNavigation;
    m_realNavigation = false;
    if (m_webView && m_currentTab->valid() && !realNavigation && url() != "about:blank") {
        QMetaObject::invokeMethod(this, "load", Qt::DirectConnection,
                                  Q_ARG(QVariant, url()),
                                  Q_ARG(QVariant, title()),
                                  Q_ARG(QVariant, false));
    }
}

/**
 * @brief DeclarativeTab::captureScreen
 * Rotation transformation is applied first, then geometry values on top of it.
 * @param url
 * @param size
 * @param rotate clockwise rotation of the image in degrees
 */
void DeclarativeWebContainer::captureScreen(QString url, int size, qreal rotate)
{
    if (!window() || !window()->isActive() || !m_currentTab->valid()) {
        return;
    }

    // Cleanup old thumb.
    m_currentTab->setThumbnailPath("");

    QImage image = window()->grabWindow();
    QRect cropBounds(0, 0, size, size);

#ifdef DEBUG_LOGS
    qDebug() << "about to set future";
#endif
    // asynchronous save to avoid the slow I/O
    m_screenCapturer.setFuture(QtConcurrent::run(this, &DeclarativeWebContainer::saveToFile, url, image, cropBounds, m_currentTab->tabId(), rotate));
}
