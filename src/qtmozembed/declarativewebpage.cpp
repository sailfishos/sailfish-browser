/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "declarativewebpage.h"
#include "declarativewebcontainer.h"
#include "dbmanager.h"
#include "browserpaths.h"
#include "logging.h"

#include <webenginesettings.h>
#include <qmozwindow.h>
#include <QGuiApplication>
#include <QtConcurrent>
#include <qmozsecurity.h>

#define FULLSCREEN_MESSAGE "embed:fullscreenchanged"
#define DOM_CONTENT_LOADED_MESSAGE "chrome:contentloaded"
#define CONTENT_ORIENTATION_CHANGED_MESSAGE "embed:contentOrientationChanged"
#define LINK_ADDED_MESSAGE "chrome:linkadded"
#define FIND_MESSAGE "embed:find"

bool isBlack(QRgb rgb)
{
    return qRed(rgb) == 0 && qGreen(rgb) == 0 && qBlue(rgb) == 0;
}

bool allBlack(const QImage &image)
{
    int h = image.height();
    int w = image.width();

    for (int j = 0; j < h; ++j) {
        const QRgb *b = (const QRgb *)image.constScanLine(j);
        for (int i = 0; i < w; ++i) {
            if (!isBlack(b[i]))
                return false;
        }
    }
    return true;
}

DeclarativeWebPage::DeclarativeWebPage(QObject *parent)
    : QOpenGLWebPage(parent)
    , m_container(0)
    , m_userHasDraggedWhileLoading(false)
    , m_fullscreen(false)
    , m_forcedChrome(false)
    , m_domContentLoaded(false)
    , m_initialLoadHasHappened(false)
    , m_tabHistoryReady(false)
    , m_urlReady(false)
    , m_restoredCurrentLinkId(-1)
    , m_fullScreenHeight(0.f)
    , m_toolbarHeight(0.f)
    , m_virtualKeyboardMargin(0.f)
    , m_marginChangeThrottleTimer(0)
{

    // subscribe to gecko messages
    std::vector<std::string> messages = { FULLSCREEN_MESSAGE,
                                          DOM_CONTENT_LOADED_MESSAGE,
                                          LINK_ADDED_MESSAGE,
                                          FIND_MESSAGE,
                                          CONTENT_ORIENTATION_CHANGED_MESSAGE };

    loadFrameScript("chrome://embedlite/content/embedhelper.js");
    addMessageListeners(messages);

    connect(this, &DeclarativeWebPage::recvAsyncMessage,
            this, &DeclarativeWebPage::onRecvAsyncMessage);
    connect(&m_grabWritter, &QFutureWatcher<QString>::finished, this, &DeclarativeWebPage::grabWritten);
    connect(this, &DeclarativeWebPage::urlChanged, this, &DeclarativeWebPage::onUrlChanged);
    connect(this, &QOpenGLWebPage::contentHeightChanged, this, &DeclarativeWebPage::updateViewMargins);
    connect(this, &QOpenGLWebPage::loadedChanged, [this]() {
        if (loaded()) {
            qCDebug(lcCoreLog) << "WebPage: loaded";
            updateViewMargins();
            // E.g. when loading images directly we don't necessarily get domContentLoaded message from engine.
            // So mark content loaded when webpage is loaded.
            setContentLoaded();
        }
    });

    // When loading start reset state of chrome.
    connect(this, &QOpenGLWebPage::loadingChanged, [this]() {
        if (loading()) {
            forceChrome(false);
            setChrome(true);
        }
    });

    connect(this, &DeclarativeWebPage::fullscreenHeightChanged,
            this, [this]() {
        qCDebug(lcCoreLog) << "WebPage: fullscreenHeightChanged";
        updateViewMargins();
    });
}

DeclarativeWebPage::~DeclarativeWebPage()
{
    m_grabWritter.cancel();
    m_grabWritter.waitForFinished();
    m_grabResult.clear();
    m_thumbnailResult.clear();
}

DeclarativeWebContainer *DeclarativeWebPage::container() const
{
    return m_container;
}

void DeclarativeWebPage::setContainer(DeclarativeWebContainer *container)
{
    if (m_container != container) {
        m_container = container;
        if (m_container) {
            connect(m_container.data(), &DeclarativeWebContainer::portraitChanged,
                    this, &DeclarativeWebPage::sendVkbOpenCompositionMetrics);
        }
        Q_ASSERT(container->mozWindow());
        setMozWindow(container->mozWindow());
        emit containerChanged();
    }
}

int DeclarativeWebPage::tabId() const
{
    return m_initialTab.tabId();
}

void DeclarativeWebPage::setInitialTab(const Tab& tab)
{
    Q_ASSERT(m_initialTab.tabId() == 0);

    m_initialTab = tab;
    emit tabIdChanged();
    connect(DBManager::instance(), &DBManager::tabHistoryAvailable,
            this, &DeclarativeWebPage::onTabHistoryAvailable);
    DBManager::instance()->getTabHistory(tabId());
}

void DeclarativeWebPage::onUrlChanged()
{
    disconnect(this, &DeclarativeWebPage::urlChanged, this, &DeclarativeWebPage::onUrlChanged);
    m_urlReady = true;
    restoreHistory();
}

void DeclarativeWebPage::onTabHistoryAvailable(const int& historyTabId, const QList<Link>& links, int currentLinkId)
{
    if (historyTabId == tabId()) {
        m_restoredTabHistory = links;
        m_restoredCurrentLinkId = currentLinkId; // FIXME: consider storing isCurrent flag in Link struct instead to reduce DeclarativeWebPage's state

        std::reverse(m_restoredTabHistory.begin(), m_restoredTabHistory.end());
        DBManager::instance()->disconnect(this);
        m_tabHistoryReady = true;
        restoreHistory();
    }
}

void DeclarativeWebPage::restoreHistory() {
    if (!m_urlReady || !m_tabHistoryReady || m_restoredTabHistory.count() == 0) {
        return;
    }

    QList<QString> urls;
    int index(-1);
    int i(0);
    foreach (Link link, m_restoredTabHistory) {
        urls << link.url();
        if (link.linkId() == m_restoredCurrentLinkId) {
            index = i;
            QString currentUrl(url().toString());
            if (link.url() != currentUrl) {
                // The browser was started with an initial URL as a cmdline parameter -> reset tab history
                urls << currentUrl;
                index++;
                DBManager::instance()->navigateTo(tabId(), currentUrl, "", "");
                break;
            }
        }
        i++;
    }

    if (index < 0) {
        urls << url().toString();
        index = urls.count() - 1;
    }

    QVariantMap data;
    data.insert(QString("links"), QVariant(urls));
    data.insert(QString("index"), QVariant(index));
    sendAsyncMessage("embedui:addhistory", QVariant(data));

    // History is restored once per webpage's life cycle.
    m_restoredTabHistory.clear();
}

void DeclarativeWebPage::setContentLoaded()
{
    if (!m_domContentLoaded) {
        m_domContentLoaded = true;
        emit domContentLoadedChanged();
    }
}

bool DeclarativeWebPage::domContentLoaded() const
{
    return m_domContentLoaded;
}

bool DeclarativeWebPage::initialLoadHasHappened() const
{
    return m_initialLoadHasHappened;
}

void DeclarativeWebPage::setInitialLoadHasHappened()
{
    m_initialLoadHasHappened = true;
}

QVariant DeclarativeWebPage::resurrectedContentRect() const
{
    return m_resurrectedContentRect;
}

void DeclarativeWebPage::setResurrectedContentRect(QVariant resurrectedContentRect)
{
    if (m_resurrectedContentRect != resurrectedContentRect) {
        m_resurrectedContentRect = resurrectedContentRect;
        emit resurrectedContentRectChanged();
    }
}

qreal DeclarativeWebPage::toolbarHeight() const
{
    return m_toolbarHeight;
}

void DeclarativeWebPage::setToolbarHeight(qreal toolbarHeight)
{
    if (toolbarHeight != m_toolbarHeight) {
        m_toolbarHeight = toolbarHeight;
        emit toolbarHeightChanged();
    }
}

qreal DeclarativeWebPage::virtualKeyboardMargin() const
{
    return m_virtualKeyboardMargin;
}

void DeclarativeWebPage::setVirtualKeyboardMargin(qreal margin)
{
    qCDebug(lcCoreLog) << "WebPage: setting vkb margins:" << margin;
    if (margin != m_virtualKeyboardMargin) {
        m_virtualKeyboardMargin = margin;
        if (m_virtualKeyboardMargin == 0.0) {
            // Only place where we ignore view margins update guards.
            // It must be allowed to close vkb while content is moving.
            resetViewMargins();
        } else {
            QMargins margins;
            margins.setBottom(m_virtualKeyboardMargin);
            qCDebug(lcCoreLog) << "WebPage: set vkb margins:" << m_virtualKeyboardMargin;
            setMargins(margins);
        }
        sendVkbOpenCompositionMetrics();
        emit virtualKeyboardMarginChanged();
    }
}

void DeclarativeWebPage::loadTab(const QString &newUrl, bool force)
{
    // Always enable chrome when load is called.
    setChrome(true);
    QString oldUrl = url().toString();
    if ((!newUrl.isEmpty() && oldUrl != newUrl) || force) {
        m_domContentLoaded = false;
        emit domContentLoadedChanged();
        load(newUrl);
    }
}

void DeclarativeWebPage::grabToFile(const QSize &size)
{
    // grabToImage handles invalid geometry.
    m_grabResult = grabToImage(size);
    if (m_grabResult && active()) {
        if (!m_grabResult->isReady()) {
            connect(m_grabResult.data(), &QMozGrabResult::ready,
                    this, &DeclarativeWebPage::grabResultReady);
        } else {
            grabResultReady();
        }
    } else {
        m_grabResult.clear();
    }
}


void DeclarativeWebPage::grabThumbnail(const QSize &size)
{
    m_thumbnailResult = grabToImage(size);
    if (m_thumbnailResult && active()) {
        connect(m_thumbnailResult.data(), &QMozGrabResult::ready,
                this, &DeclarativeWebPage::thumbnailReady);
    } else {
        m_thumbnailResult.clear();
    }
}

/**
 * Use this to lock to chrome mode. This disables the gesture
 * that normally enables fullscreen mode. The chromeGestureEnabled property
 * is bound to this so that contentHeight changes do not re-enable the
 * gesture.
 *
 * When gesture is allowed to be used again, unlock call by forceChrome(false).
 *
 * Used for instance when find-in-page view is active that is part of
 * the new browser user interface.
 */
void DeclarativeWebPage::forceChrome(bool forcedChrome)
{
    qCDebug(lcCoreLog) << "WebPage: force chrome:" << forcedChrome;

    // This way we don't break chromeGestureEnabled and chrome bindings.
    setChromeGestureEnabled(!forcedChrome);
    if (forcedChrome) {
        setChrome(forcedChrome);
    }
    if (m_forcedChrome != forcedChrome) {
        m_forcedChrome = forcedChrome;
        emit forcedChromeChanged();
    }
}

void DeclarativeWebPage::timerEvent(QTimerEvent *te)
{
    if (te->timerId() == m_marginChangeThrottleTimer) {
        killTimer(m_marginChangeThrottleTimer);
        m_marginChangeThrottleTimer = 0;

    }
    QOpenGLWebPage::timerEvent(te);
}

void DeclarativeWebPage::grabResultReady()
{
    if (active()) {
        QImage image = m_grabResult->image();
        m_grabWritter.setFuture(QtConcurrent::run(this, &DeclarativeWebPage::saveToFile, image));
    }
    m_grabResult.clear();
}

void DeclarativeWebPage::grabWritten()
{
    QString path = m_grabWritter.result();
    emit grabResult(path);
}

void DeclarativeWebPage::thumbnailReady()
{
    if (active()) {
        QImage image = m_thumbnailResult->image();
        QByteArray iconData;
        QBuffer buffer(&iconData);
        buffer.open(QIODevice::WriteOnly);
        if (image.save(&buffer, "jpg", 75)) {
            buffer.close();
            emit thumbnailResult(QString(BASE64_IMAGE).arg(QString(iconData.toBase64())));
        } else {
            emit thumbnailResult(DEFAULT_DESKTOP_BOOKMARK_ICON);
        }
    }
    m_thumbnailResult.clear();
}

void DeclarativeWebPage::updateViewMargins()
{
    qCDebug(lcCoreLog) << "WebPage: update view margins, foreground:" << (m_container && !m_container->foreground()) << "throttling:" << (m_marginChangeThrottleTimer > 0) << "moving:" << moving();

    // Don't update margins while panning, flicking, pinching, vkb is already open, or
    // margin update is ongoing (throttling).
    if ((m_container && !m_container->foreground()) || m_marginChangeThrottleTimer > 0 ||
            moving() || m_virtualKeyboardMargin > 0) {
        return;
    }

    resetViewMargins();
}

void DeclarativeWebPage::resetViewMargins()
{
    qCDebug(lcCoreLog) << "WebPage: reset view margins, fullscreen:" << m_fullscreen << "content height:" << contentHeight();

    // Reset margins always when fullscreen mode is enabled.
    QMargins margins;
    if (!m_fullscreen) {
        qreal threshold = qMax(m_fullScreenHeight * 1.5f, (m_fullScreenHeight + (m_toolbarHeight*2)));
        if (contentHeight() < threshold) {
            margins.setBottom(m_toolbarHeight);
        }
    }

    // Some content needed so that it makes sense to throttle content height changes.
    if (contentHeight() > 0) {
        m_marginChangeThrottleTimer = startTimer(200);
    }

    qCDebug(lcCoreLog) << "WebPage: set margins:" << margins;
    setMargins(margins);
}

void DeclarativeWebPage::sendVkbOpenCompositionMetrics()
{
    // Send update only if the page is active.
    if (!active()) {
        return;
    }

    int winHeight(0);
    int winWidth(0);

    // Listen im state so that we don't send also updates when
    // vkb state changes on the chrome side.

    if (m_container) {
      if (m_container->portrait()) {
        winHeight = m_container->height();
        winWidth = m_container->width();
      } else {
        winHeight = m_container->width();
        winWidth = m_container->height();
      }
    }

    QVariantMap map;
    map.insert("imOpen", m_virtualKeyboardMargin > 0);
    map.insert("pixelRatio", SailfishOS::WebEngineSettings::instance()->pixelRatio());
    map.insert("bottomMargin", m_virtualKeyboardMargin);
    map.insert("screenWidth", winWidth);
    map.insert("screenHeight", winHeight);

    QVariant data(map);
    sendAsyncMessage("embedui:vkbOpenCompositionMetrics", data);
}

QString DeclarativeWebPage::saveToFile(QImage image)
{
    if (image.isNull() || !active()) {
        return "";
    }

    // 75% quality jpg produces small and good enough capture.
    QString path = QString("%1/tab-%2-thumb.jpg").arg(BrowserPaths::cacheLocation()).arg(tabId());
    return !allBlack(image) && image.save(path, "jpg", 75) ? path : "";
}

void DeclarativeWebPage::onRecvAsyncMessage(const QString& message, const QVariant& data)
{
    if (message == QLatin1String(FULLSCREEN_MESSAGE)) {
        setFullscreen(data.toMap().value(QString("fullscreen")).toBool());
    } else if (message == QLatin1String(DOM_CONTENT_LOADED_MESSAGE)) {
        setContentLoaded();
    } else if (message == QLatin1String(CONTENT_ORIENTATION_CHANGED_MESSAGE)) {
        QString orientation = data.toMap().value(QStringLiteral("orientation")).toString();
        Qt::ScreenOrientation mappedOrientation = Qt::PortraitOrientation;
        if (orientation == QLatin1String("landscape-primary")) {
            mappedOrientation = Qt::LandscapeOrientation;
        } else if (orientation == QLatin1String("landscape-secondary")) {
            mappedOrientation = Qt::InvertedLandscapeOrientation;
        } else if (orientation == QLatin1String("portrait-secondary")) {
            mappedOrientation = Qt::InvertedPortraitOrientation;
        }
        emit contentOrientationChanged(mappedOrientation);
    }
}

bool DeclarativeWebPage::fullscreen() const
{
    return m_fullscreen;
}

bool DeclarativeWebPage::forcedChrome() const
{
    return m_forcedChrome;
}

void DeclarativeWebPage::setFullscreen(const bool fullscreen)
{
    if (m_fullscreen != fullscreen) {
        m_fullscreen = fullscreen;
        qCDebug(lcCoreLog) << "WebPage: fullscreen:" << fullscreen;
        resetViewMargins();
        emit fullscreenChanged();
    }
}

QDebug operator<<(QDebug dbg, const DeclarativeWebPage *page)
{
    if (!page) {
        return dbg << "DeclarativeWebPage (this = 0x0)";
    }

    QSize size = page->mozWindow()->size();
    dbg.nospace() << "DeclarativeWebPage(tabId = " << page->tabId() << " url = " << page->url() << ", title = " << page->title() << ", width = " << size.width()
                  << ", height = " << size.height() << ", completed = " << page->completed()
                  << ", active = " << page->active() << ", enabled = " << page->enabled() << ")";
    return dbg.space();
}

