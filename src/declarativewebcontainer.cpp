/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

#include "declarativewebcontainer.h"

#include <QPointer>
#include <QMetaObject>
#include <QTimerEvent>
#include <QQuickWindow>

DeclarativeWebContainer::DeclarativeWebContainer(QQuickItem *parent)
    : QQuickItem(parent)
    , m_webView(0)
    , m_foreground(true)
    , m_background(false)
    , m_windowVisible(false)
    , m_backgroundTimer(0)
    , m_pageActive(false)
    , m_inputPanelVisible(false)
    , m_inputPanelHeight(0.0)
    , m_inputPanelOpenHeight(0.0)
    , m_toolbarHeight(0.0)
{
    setFlag(QQuickItem::ItemHasContents, true);
    if (!window()) {
        connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
    } else {
        connect(window(), SIGNAL(visibleChanged(bool)), this, SLOT(windowVisibleChanged(bool)));
    }
}

DeclarativeWebContainer::~DeclarativeWebContainer()
{
    // Disconnect all signal slot connections
    if (m_webView) {
        disconnect(m_webView, 0, 0, 0);
    }
}

QQuickItem *DeclarativeWebContainer::webView() const
{
    return m_webView;
}

void DeclarativeWebContainer::setWebView(QQuickItem *webView)
{
    if (m_webView != webView) {
        m_webView = webView;

        connect(m_webView, SIGNAL(imeNotification(int,bool,int,int,QString)),
                this, SLOT(imeNotificationChanged(int,bool,int,int,QString)));
        connect(m_webView, SIGNAL(contentHeightChanged()), this, SLOT(resetHeight()));
        connect(m_webView, SIGNAL(scrollableOffsetChanged()), this, SLOT(resetHeight()));
        connect(this, SIGNAL(heightChanged()), this, SLOT(resetHeight()));
        emit webViewChanged();
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

bool DeclarativeWebContainer::pageActive() const
{
    return m_pageActive;
}

void DeclarativeWebContainer::setPageActive(bool active)
{
    if (m_pageActive != active) {
        m_pageActive = active;
        emit pageActiveChanged();

        // If dialog has been opened, we need to verify that input panel is not visible.
        // This might happen when the user fills in login details to a form and
        // presses enter to accept the form after which PasswordManagerDialog is pushed to pagestack
        // on top the BrowserPage. Once PassowordManagerDialog is accepted/rejected
        // this condition can be met. If pageActive changes to true before keyboard is fully closed,
        // then the inputPanelVisibleChanged() signal is emitted by setInputPanelHeight.
        if (m_pageActive && m_inputPanelHeight == 0 && m_inputPanelVisible) {
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
        if (m_pageActive) {
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

qreal DeclarativeWebContainer::inputPanelOpenHeight() const
{
    return m_inputPanelOpenHeight;
}

void DeclarativeWebContainer::setInputPanelOpenHeight(qreal height)
{
    if (m_inputPanelOpenHeight != height) {
        m_inputPanelOpenHeight = height;
        emit inputPanelOpenHeightChanged();
    }
}

qreal DeclarativeWebContainer::toolbarHeight() const
{
    return m_toolbarHeight;
}

void DeclarativeWebContainer::setToolbarHeight(qreal height)
{
    if (m_toolbarHeight != height) {
        m_toolbarHeight = height;
        emit toolbarHeightChanged();
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
    static QMetaProperty property;

    if (m_webView) {
        if (!property.isValid()) {
            const QMetaObject *webViewMetaObject = m_webView->metaObject();
            int propertyIndex = webViewMetaObject->indexOfProperty("contentHeight");
            property = webViewMetaObject->property(propertyIndex);
        }

        qreal height = property.read(m_webView).toReal();
        return height;
    } else {
        return 0.0;
    }
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


void DeclarativeWebContainer::windowVisibleChanged(bool visible)
{
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
