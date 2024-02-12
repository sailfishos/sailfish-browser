/*
 * Copyright (c) 2014 - 2021 Jolla Ltd.
 * Copyright (c) 2019 - 2021 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "declarativewebpage.h"
#include "declarativewebcontainer.h"
#include "dbmanager.h"
#include "browserapp.h"
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
#define LINK_SET_ICON "Link:SetIcon"
#define LINK_ADD_FEED "Link:AddFeed"
#define LINK_ADD_SEARCH "Link:AddSearch"
#define FIND_MESSAGE "embed:find"
#define OPEN_LINK "embed:OpenLink"

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
    , m_tabHistoryReady(false)
    , m_urlReady(false)
    , m_restoredCurrentLinkId(-1)
    , m_fullScreenHeight(0.f)
    , m_toolbarHeight(0.f)
{

    // subscribe to gecko messages
    std::vector<std::string> messages = { FULLSCREEN_MESSAGE,
                                          DOM_CONTENT_LOADED_MESSAGE,
                                          LINK_SET_ICON,
                                          LINK_ADD_FEED,
                                          LINK_ADD_SEARCH,
                                          FIND_MESSAGE,
                                          CONTENT_ORIENTATION_CHANGED_MESSAGE };

    addMessageListeners(messages);

    if (BrowserApp::captivePortal()) {
        addMessageListener(OPEN_LINK);
        loadFrameScript("file:///usr/share/sailfish-captiveportal/pages/captiveportal.js");
    }

    connect(this, &DeclarativeWebPage::recvAsyncMessage,
            this, &DeclarativeWebPage::onRecvAsyncMessage);
    connect(&m_grabWritter, &QFutureWatcher<QString>::finished, this, &DeclarativeWebPage::grabWritten);
    connect(this, &DeclarativeWebPage::urlChanged, this, &DeclarativeWebPage::onUrlChanged);
    connect(this, &QOpenGLWebPage::virtualKeyboardHeightChanged, this, &DeclarativeWebPage::updateViewMargins);
    connect(this, &QOpenGLWebPage::domContentLoadedChanged, [this]() {
        if (domContentLoaded()) {
            qCDebug(lcCoreLog) << "WebPage: loaded";
            updateViewMargins();
        }
    });

    // When loading start reset state of chrome.
    connect(this, &QOpenGLWebPage::loadingChanged, [this]() {
        if (loading()) {
            forceChrome(false);
            setChrome(true);
        }
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
        Q_ASSERT(container->mozWindow());
        setMozWindow(container->mozWindow());
        emit containerChanged();
    }
}

int DeclarativeWebPage::tabId() const
{
    return m_initialTab.tabId();
}

void DeclarativeWebPage::setInitialState(const Tab& tab, bool privateMode)
{
    Q_ASSERT(m_initialTab.tabId() == 0);

    setParentId(tab.parentId());
    setPrivateMode(privateMode);
    setParentBrowsingContext(tab.browsingContext());

    m_initialTab = tab;
    setDesktopMode(m_initialTab.desktopMode());
    emit tabIdChanged();
    connect(DBManager::instance(), &DBManager::tabHistoryAvailable,
            this, &DeclarativeWebPage::onTabHistoryAvailable);
}

void DeclarativeWebPage::onUrlChanged()
{
    // Only update resolved urls for navigation history.
    bool urlResolved = isUrlResolved();
    if (urlResolved) {
        emit updateUrl();
    }

    // Get tab history only after first url is resolved.
    // Above urlResolved guarantees that we have url resolved
    // by the engine.
    bool urlReadyChanged = !m_urlReady;
    if (urlReadyChanged && urlResolved) {
        m_urlReady = true;
        DBManager::instance()->getTabHistory(tabId());
    }
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
    for (const Link &link : m_restoredTabHistory) {
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
        updateViewMargins();
        emit toolbarHeightChanged();
    }
}

void DeclarativeWebPage::loadTab(const QString &newUrl, bool force, bool fromExternal)
{
    // Always enable chrome when load is called.
    setChrome(true);
    QString oldUrl = url().toString();
    if ((!newUrl.isEmpty() && oldUrl != newUrl) || force) {
        load(newUrl, fromExternal);
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

void DeclarativeWebPage::grabResultReady()
{
    QImage image = m_grabResult->image();
    if (!image.isNull() && active()) {
        m_grabWritter.setFuture(QtConcurrent::run(
                &DeclarativeWebPage::saveToFile,
                image,
                QStringLiteral("%1/tab-%2-thumb.jpg").arg(BrowserPaths::cacheLocation()).arg(tabId())));
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
    qreal toolbarHeight = virtualKeyboardHeight() ? 0.0 : m_toolbarHeight;
    qCDebug(lcCoreLog) << "WebPage: set dynamic toolbar height:" << toolbarHeight;
    setDynamicToolbarHeight(toolbarHeight);

    QMargins margins;
    margins.setBottom(virtualKeyboardHeight());

    qCDebug(lcCoreLog) << "WebPage: set margins:" << margins;
    setMargins(margins);
}

QString DeclarativeWebPage::saveToFile(const QImage &image, const QString &path)
{
    if (allBlack(image)) {
        return QString();
    }

    QSaveFile saveFile(path);
    if (image.save(&saveFile, "jpg", 75)) {
        saveFile.commit();

        return path;
    } else {
        return QString();
    }
}

void DeclarativeWebPage::onRecvAsyncMessage(const QString& message, const QVariant& data)
{
    if (message == QLatin1String(FULLSCREEN_MESSAGE)) {
        setFullscreen(data.toMap().value(QString("fullscreen")).toBool());
    } else if (message == QLatin1String(DOM_CONTENT_LOADED_MESSAGE)) {
        QString docuri = data.toMap().value("docuri").toString();
        if (docuri.startsWith("about:neterror") && !docuri.contains("e=netOffline"))
            emit neterror();
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
        updateViewMargins();
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

