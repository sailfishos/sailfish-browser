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
#include "browserappinfo.h"
#include "browserpaths.h"
#include "faviconmanager.h"
#include "logging.h"

#include <qmozwindow.h>
#include <qmozsecurity.h>

#include <QBuffer>
#include <QGuiApplication>
#include <QScreen>
#include <QtConcurrent>

#define FULLSCREEN_MESSAGE "embed:fullscreenchanged"
#define DOM_CONTENT_LOADED_MESSAGE "chrome:contentloaded"
#define PAGE_METADATA_MESSAGE "embed:pageMetadata"
#define CONTENT_ORIENTATION_CHANGED_MESSAGE "embed:contentOrientationChanged"
#define VIEWPORT_FIT_MESSAGE "embed:viewportfit"
#define LINK_SET_ICON "Link:SetIcon"
#define LINK_ADD_FEED "Link:AddFeed"
#define LINK_ADD_SEARCH "Link:AddSearch"
#define FIND_MESSAGE "embed:find"
#define OPEN_LINK "embed:OpenLink"

static bool isBlack(QRgb rgb)
{
    return qRed(rgb) == 0 && qGreen(rgb) == 0 && qBlue(rgb) == 0;
}

static bool allBlack(const QImage &image)
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
    : QMozOpenGLWebPage(parent)
    , m_container(0)
    , m_fullscreen(false)
    , m_forcedChrome(false)
    , m_tabHistoryReady(false)
    , m_urlReady(false)
    , m_restoredCurrentLinkId(-1)
    , m_toolbarHeight(0.f)
    , m_viewportFit(QStringLiteral("auto"))
    , m_safeAreaInsetUsage(0)
{
    // subscribe to gecko messages
    std::vector<std::string> messages = { FULLSCREEN_MESSAGE,
                                          DOM_CONTENT_LOADED_MESSAGE,
                                          PAGE_METADATA_MESSAGE,
                                          LINK_SET_ICON,
                                          LINK_ADD_FEED,
                                          LINK_ADD_SEARCH,
                                          FIND_MESSAGE,
                                          CONTENT_ORIENTATION_CHANGED_MESSAGE,
                                          VIEWPORT_FIT_MESSAGE };

    addMessageListeners(messages);
    const QString sharedPath = QStringLiteral("file:///usr/share/%1/shared")
            .arg(BrowserAppInfo::captivePortal()
                 ? QStringLiteral("sailfish-captiveportal")
                 : QStringLiteral("sailfish-browser"));
    loadFrameScript(sharedPath + QStringLiteral("/ViewportFit.js"));
    loadFrameScript(sharedPath + QStringLiteral("/PageMetadata.js"));

    if (BrowserAppInfo::captivePortal()) {
        addMessageListener(OPEN_LINK);
        loadFrameScript("file:///usr/share/sailfish-captiveportal/pages/captiveportal.js");
    }

    connect(this, &DeclarativeWebPage::recvAsyncMessage,
            this, &DeclarativeWebPage::onRecvAsyncMessage);
    connect(&m_grabWriter, &QFutureWatcher<QString>::finished, this, &DeclarativeWebPage::handleFileGrabFile);
    connect(this, &DeclarativeWebPage::urlChanged, this, &DeclarativeWebPage::onUrlChanged);
    connect(this, &QMozOpenGLWebPage::domContentLoadedChanged, [this]() {
        if (domContentLoaded()) {
            qCDebug(lcCoreLog) << "WebPage: loaded";
            updateViewMargins();
        }
    });

    // When loading start reset state of chrome.
    connect(this, &QMozOpenGLWebPage::loadingChanged, [this]() {
        if (loading()) {
            setViewportFit(QStringLiteral("auto"));
            setSafeAreaInsetUsage(0);
            setThemeColor(QColor(Qt::black));
            forceChrome(false);
            setChrome(true);
        }
    });
}

DeclarativeWebPage::~DeclarativeWebPage()
{
    m_grabWriter.cancel();
    m_grabWriter.waitForFinished();
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
        QMozWindow * const window = new QMozWindow(container->webContentSize());
        if (container->screen()) {
            window->setPrimaryOrientation(
                    container->screen()->primaryOrientation());
        }
        setMozWindow(window);
        if (!mozWindow()) {
            qCWarning(lcCoreLog) << "Failed to create hosted page window";
        }
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

    const QString initialUrl = !tab.url().isEmpty()
            ? tab.url() : !tab.requestedUrl().isEmpty()
              ? tab.requestedUrl() : QStringLiteral("about:blank");
    load(initialUrl, false);

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
        // FIXME: consider storing isCurrent flag in Link struct instead to reduce DeclarativeWebPage's state
        m_restoredCurrentLinkId = currentLinkId;

        std::reverse(m_restoredTabHistory.begin(), m_restoredTabHistory.end());
        DBManager::instance()->disconnect(this);
        m_tabHistoryReady = true;
        restoreHistory();
    }
}

void DeclarativeWebPage::restoreHistory()
{
    if (!m_urlReady || !m_tabHistoryReady || m_restoredTabHistory.count() == 0) {
        return;
    }

    QList<QString> urls;
    int index = -1;
    int i = 0;

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

int DeclarativeWebPage::virtualKeyboardHeight() const
{
    return m_virtualKeyboardHeight;
}

void DeclarativeWebPage::setVirtualKeyboardHeight(int height)
{
    if (m_virtualKeyboardHeight != height) {
        m_virtualKeyboardHeight = height;
        updateViewMargins();
        emit virtualKeyboardHeightChanged();
    }
}

void DeclarativeWebPage::setToolbarHeight(qreal toolbarHeight)
{
    if (toolbarHeight != m_toolbarHeight) {
        m_toolbarHeight = toolbarHeight;
        updateViewMargins();
        emit toolbarHeightChanged();
    }
}

bool DeclarativeWebPage::fixedToolbar() const
{
    return m_fixedToolbar;
}

void DeclarativeWebPage::setFixedToolbar(bool enable)
{
    if (enable != m_fixedToolbar) {
        m_fixedToolbar = enable;
        updateChromeState();

        emit fixedToolbarChanged();
    }
}

int DeclarativeWebPage::safeAreaTop() const
{
    return m_safeAreaInsets.top();
}

void DeclarativeWebPage::setSafeAreaTop(int top)
{
    applySafeAreaInsets(QMargins(m_safeAreaInsets.left(), qMax(0, top),
                                 m_safeAreaInsets.right(), m_safeAreaInsets.bottom()));
}

int DeclarativeWebPage::safeAreaRight() const
{
    return m_safeAreaInsets.right();
}

void DeclarativeWebPage::setSafeAreaRight(int right)
{
    applySafeAreaInsets(QMargins(m_safeAreaInsets.left(), m_safeAreaInsets.top(),
                                 qMax(0, right), m_safeAreaInsets.bottom()));
}

int DeclarativeWebPage::safeAreaBottom() const
{
    return m_safeAreaInsets.bottom();
}

void DeclarativeWebPage::setSafeAreaBottom(int bottom)
{
    applySafeAreaInsets(QMargins(m_safeAreaInsets.left(), m_safeAreaInsets.top(),
                                 m_safeAreaInsets.right(), qMax(0, bottom)));
}

int DeclarativeWebPage::safeAreaLeft() const
{
    return m_safeAreaInsets.left();
}

void DeclarativeWebPage::setSafeAreaLeft(int left)
{
    applySafeAreaInsets(QMargins(qMax(0, left), m_safeAreaInsets.top(),
                                 m_safeAreaInsets.right(), m_safeAreaInsets.bottom()));
}

void DeclarativeWebPage::applySafeAreaInsets(const QMargins &insets)
{
    if (m_safeAreaInsets != insets) {
        m_safeAreaInsets = insets;
        setSafeAreaInsets(insets);
        emit safeAreaInsetsChanged();
    }
}

QString DeclarativeWebPage::viewportFit() const
{
    return m_viewportFit;
}

void DeclarativeWebPage::setViewportFit(const QString &viewportFit)
{
    QString normalized = viewportFit;
    if (normalized != QLatin1String("cover") && normalized != QLatin1String("contain")) {
        normalized = QStringLiteral("auto");
    }

    if (m_viewportFit != normalized) {
        m_viewportFit = normalized;
        qCDebug(lcCoreLog) << "WebPage: viewport fit:" << normalized;
        emit viewportFitChanged();
    }
}

int DeclarativeWebPage::safeAreaInsetUsage() const
{
    return m_safeAreaInsetUsage;
}

void DeclarativeWebPage::setSafeAreaInsetUsage(int usage)
{
    int normalized = usage & 0x0f;
    if (m_safeAreaInsetUsage != normalized) {
        m_safeAreaInsetUsage = normalized;
        qCDebug(lcCoreLog) << "WebPage: safe area inset usage:" << normalized;
        emit safeAreaInsetUsageChanged();
    }
}

QColor DeclarativeWebPage::themeColor() const
{
    return m_themeColor;
}

bool DeclarativeWebPage::hasThemeColor() const
{
    return m_themeColor.isValid();
}

void DeclarativeWebPage::setThemeColor(const QColor &color)
{
    QColor normalized = color;
    if (normalized.isValid()) {
        normalized.setAlpha(255);
    }

    if (m_themeColor != normalized) {
        m_themeColor = normalized;
        qCDebug(lcCoreLog) << "WebPage: theme color:"
                            << (normalized.isValid() ? normalized.name() : QStringLiteral("none"));
        emit themeColorChanged();
    }
}

void DeclarativeWebPage::updateChromeState()
{
    if (m_forcedChrome || m_fixedToolbar) {
        setChrome(true);
    }

    updateViewMargins();
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

void DeclarativeWebPage::updateMetadataTitle(const QString &title)
{
    const QString currentUrl = url().toString();
    const QString cleanedTitle = title.trimmed();
    if (privateMode() || BrowserAppInfo::captivePortal() || !isUrlResolved()
            || currentUrl.startsWith(QLatin1String("about:")) || cleanedTitle.isEmpty()) {
        return;
    }

    DBManager::instance()->updateTitle(tabId(), currentUrl, cleanedTitle);
}

void DeclarativeWebPage::grabToFile(const QSize &size)
{
    if (m_container) {
        m_grabResult.clear();
        const QImage image = m_container->grabContentImage(size);
        if (!image.isNull() && active()) {
            m_grabWriter.setFuture(QtConcurrent::run(
                    &DeclarativeWebPage::saveToFile,
                    image,
                    QStringLiteral("%1/tab-%2-thumb.jpg").arg(BrowserPaths::cacheLocation()).arg(tabId())));
        }
        return;
    }

    // grabToImage handles invalid geometry.
    m_grabResult = grabToImage(size);
    if (m_grabResult && active()) {
        if (!m_grabResult->isReady()) {
            connect(m_grabResult.data(), &QMozGrabResult::ready,
                    this, &DeclarativeWebPage::handleFileGrabImage);
        } else {
            handleFileGrabImage();
        }
    } else {
        m_grabResult.clear();
    }
}

void DeclarativeWebPage::grabThumbnail(const QSize &size)
{
    if (m_container) {
        m_thumbnailResult.clear();
        const QImage image = m_container->grabContentImage(size);
        if (!image.isNull() && active()) {
            QByteArray iconData;
            QBuffer buffer(&iconData);
            buffer.open(QIODevice::WriteOnly);
            if (image.save(&buffer, "jpg", 75)) {
                buffer.close();
                emit thumbnailResult(QStringLiteral("data:image/jpeg;base64,")
                                     + QString::fromLatin1(iconData.toBase64()));
            } else {
                emit thumbnailResult(FaviconManager::defaultDesktopBookmarkIcon());
            }
        }
        return;
    }

    m_thumbnailResult = grabToImage(size);
    if (m_thumbnailResult && active()) {
        connect(m_thumbnailResult.data(), &QMozGrabResult::ready,
                this, &DeclarativeWebPage::thumbnailReady);
    } else {
        m_thumbnailResult.clear();
    }
}

/**
 * Use this to temporarily force chrome visible.
 *
 * When chrome hiding gesture is allowed to be used again, unlock call by forceChrome(false).
 *
 * Used for instance when find-in-page view is active that is part of
 * the new browser user interface.
 */
void DeclarativeWebPage::forceChrome(bool forcedChrome)
{
    qCDebug(lcCoreLog) << "WebPage: force chrome:" << forcedChrome;

    if (m_forcedChrome != forcedChrome) {
        m_forcedChrome = forcedChrome;

        updateChromeState();
        emit forcedChromeChanged();
    }
}

void DeclarativeWebPage::handleFileGrabImage()
{
    QImage image = m_grabResult->image();
    if (!image.isNull() && active()) {
        m_grabWriter.setFuture(QtConcurrent::run(
                &DeclarativeWebPage::saveToFile,
                image,
                QStringLiteral("%1/tab-%2-thumb.jpg").arg(BrowserPaths::cacheLocation()).arg(tabId())));
    }
    m_grabResult.clear();
}

void DeclarativeWebPage::handleFileGrabFile()
{
    QString path = m_grabWriter.result();
    emit fileGrabWritten(path);
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
            emit thumbnailResult(QStringLiteral("data:image/jpeg;base64,")
                                 + QString::fromLatin1(iconData.toBase64()));
        } else {
            emit thumbnailResult(FaviconManager::defaultDesktopBookmarkIcon());
        }
    }
    m_thumbnailResult.clear();
}

void DeclarativeWebPage::updateViewMargins()
{
    qreal toolbarHeight = virtualKeyboardHeight() || m_forcedChrome || m_fixedToolbar ? 0.0
                                                                                      : m_toolbarHeight;
    qCDebug(lcCoreLog) << "WebPage: set dynamic toolbar height:" << toolbarHeight;
    setDynamicToolbarHeight(toolbarHeight);

    QMargins margins;
    margins.setBottom(qMax(virtualKeyboardHeight(),
                           m_forcedChrome || m_fixedToolbar ? static_cast<int>(m_toolbarHeight) : 0));

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
    } else if (message == QLatin1String(PAGE_METADATA_MESSAGE)) {
        const QVariantMap metadata = data.toMap();
        const QString metadataUrl = metadata.value(QStringLiteral("url")).toString();
        if (metadataUrl.isEmpty() || metadataUrl == url().toString()) {
            updateMetadataTitle(metadata.value(QStringLiteral("title")).toString());
        }
    } else if (message == QLatin1String(VIEWPORT_FIT_MESSAGE)) {
        QVariantMap viewportFitInfo = data.toMap();
        setViewportFit(viewportFitInfo.value(QStringLiteral("viewportFit")).toString());
        setSafeAreaInsetUsage(viewportFitInfo.value(QStringLiteral("safeAreaInsetUsage")).toInt());
        setThemeColor(QColor(viewportFitInfo.value(QStringLiteral("themeColor")).toString()));
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
    dbg.nospace() << "DeclarativeWebPage(tabId = " << page->tabId() << " url = " << page->url()
                  << ", title = " << page->title() << ", width = " << size.width()
                  << ", height = " << size.height() << ", completed = " << page->completed()
                  << ", active = " << page->active() << ", enabled = " << page->enabled() << ")";
    return dbg.space();
}
