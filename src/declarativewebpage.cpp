/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativewebpage.h"
#include "declarativewebcontainer.h"

#include <QtConcurrent>
#include <QStandardPaths>

static const QString gFullScreenMessage("embed:fullscreenchanged");
static const QString gDomContentLoadedMessage("embed:domcontentloaded");

static const QString gLinkAddedMessage("chrome:linkadded");
static const QString gAlertMessage("embed:alert");
static const QString gConfirmMessage("embed:confirm");
static const QString gPromptMessage("embed:prompt");
static const QString gAuthMessage("embed:auth");
static const QString gLoginMessage("embed:login");
static const QString gFindMessage("embed:find");
static const QString gPermissionsMessage("embed:permissions");
static const QString gContextMenuMessage("Content:ContextMenu");
static const QString gSelectionRangeMessage("Content:SelectionRange");
static const QString gSelectionCopiedMessage("Content:SelectionCopied");
static const QString gSelectAsyncMessage("embed:selectasync");
static const QString gFilePickerMessage("embed:filepicker");

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

DeclarativeWebPage::DeclarativeWebPage(QQuickItem *parent)
    : QuickMozView(parent)
    , m_container(0)
    , m_tabId(0)
    , m_viewReady(false)
    , m_userHasDraggedWhileLoading(false)
    , m_fullscreen(false)
    , m_forcedChrome(false)
    , m_domContentLoaded(false)
    , m_urlHasChanged(false)
    , m_backForwardNavigation(false)
    , m_boundToModel(false)
{
    connect(this, SIGNAL(viewInitialized()), this, SLOT(onViewInitialized()));
    connect(this, SIGNAL(recvAsyncMessage(const QString, const QVariant)),
            this, SLOT(onRecvAsyncMessage(const QString&, const QVariant&)));
    connect(&m_grabWritter, SIGNAL(finished()), this, SLOT(grabWritten()));
    connect(this, SIGNAL(contentHeightChanged()), this, SLOT(resetHeight()));
    connect(this, SIGNAL(scrollableOffsetChanged()), this, SLOT(resetHeight()));
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
        emit containerChanged();
    }
}

int DeclarativeWebPage::tabId() const
{
    return m_tabId;
}

void DeclarativeWebPage::setTabId(int tabId)
{
    if (m_tabId != tabId) {
        m_tabId = tabId;
        emit tabIdChanged();
    }
}

bool DeclarativeWebPage::domContentLoaded() const
{
    return m_domContentLoaded;
}

bool DeclarativeWebPage::urlHasChanged() const
{
    return m_urlHasChanged;
}

void DeclarativeWebPage::setUrlHasChanged(bool urlHasChanged)
{
    m_urlHasChanged = urlHasChanged;
}

void DeclarativeWebPage::setInitialUrl(const QString &url)
{
    m_initialUrl = url;
}

void DeclarativeWebPage::bindToModel()
{
    m_boundToModel = true;
}

bool DeclarativeWebPage::boundToModel()
{
    return m_boundToModel;
}

bool DeclarativeWebPage::backForwardNavigation() const
{
    return m_backForwardNavigation;
}

void DeclarativeWebPage::setBackForwardNavigation(bool backForwardNavigation)
{
    m_backForwardNavigation = backForwardNavigation;
}

bool DeclarativeWebPage::viewReady() const
{
    return m_viewReady;
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

void DeclarativeWebPage::loadTab(QString newUrl, bool force)
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

void DeclarativeWebPage::grabToFile()
{
    if (!m_viewReady || backForwardNavigation() || !active() || !isPainted())
        return;

    emit clearGrabResult();
    // grabToImage handles invalid geometry.
    m_grabResult = grabToImage();
    if (m_grabResult.data()) {
        connect(m_grabResult.data(), SIGNAL(ready()), this, SLOT(grabResultReady()));
    }
}

void DeclarativeWebPage::grabThumbnail()
{
    m_thumbnailResult = grabToImage();
    connect(m_thumbnailResult.data(), SIGNAL(ready()), this, SLOT(thumbnailReady()));
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
    // This way we don't break chromeGestureEnabled and chrome bindings.
    setChromeGestureEnabled(!forcedChrome);
    if (forcedChrome) {
        setChrome(forcedChrome);
    }
    // Without chrome respect content height.
    resetHeight(!forcedChrome);
    if (m_forcedChrome != forcedChrome) {
        m_forcedChrome = forcedChrome;
        emit forcedChromeChanged();
    }
}

void DeclarativeWebPage::resetHeight(bool respectContentHeight)
{
    if (!state().isEmpty()) {
        return;
    }

    // fullscreen() below in the fullscreen request coming from the web content.
    if (respectContentHeight && (!m_forcedChrome || fullscreen())) {
        // Handle webPage height over here, BrowserPage.qml loading
        // reset might be redundant as we have also loaded trigger
        // reset. However, I'd leave it there for safety reasons.
        // We need to reset height always back to short height when loading starts
        // so that after tab change there is always initial short composited height.
        // Height may expand when content is moved.
        if (contentHeight() > (m_fullScreenHeight + m_toolbarHeight) || fullscreen()) {
            setHeight(m_fullScreenHeight);
        } else {
            setHeight(m_fullScreenHeight - m_toolbarHeight);
        }
    } else {
        setHeight(m_fullScreenHeight - m_toolbarHeight);
    }
}

void DeclarativeWebPage::componentComplete()
{
    QuickMozView::componentComplete();
}

void DeclarativeWebPage::onViewInitialized()
{
    addMessageListener(gFullScreenMessage);
    addMessageListener(gDomContentLoadedMessage);

    addMessageListener(gLinkAddedMessage);
    addMessageListener(gAlertMessage);
    addMessageListener(gConfirmMessage);
    addMessageListener(gPromptMessage);
    addMessageListener(gAuthMessage);
    addMessageListener(gLoginMessage);
    addMessageListener(gFindMessage);
    addMessageListener(gPermissionsMessage);
    addMessageListener(gContextMenuMessage);
    addMessageListener(gSelectionRangeMessage);
    addMessageListener(gSelectionCopiedMessage);
    addMessageListener(gSelectAsyncMessage);
    addMessageListener(gFilePickerMessage);

    loadFrameScript("chrome://embedlite/content/SelectAsyncHelper.js");
    loadFrameScript("chrome://embedlite/content/embedhelper.js");

    // This is the only place that is allowed to change this to true.
    m_viewReady = true;
    emit viewReadyChanged();

    if (!m_initialUrl.isEmpty()) {
        loadTab(m_initialUrl, false);
        m_initialUrl = "";
    }
}

void DeclarativeWebPage::grabResultReady()
{
    QImage image = m_grabResult->image();
    m_grabResult.clear();
    int w = qMin(width(), height());
    int h = qMax(width(), height());
    h = qMax(h / 3, w / 2);
    QRect cropBounds(0, 0, w, h);

    m_grabWritter.setFuture(QtConcurrent::run(this, &DeclarativeWebPage::saveToFile, image, cropBounds));
}

void DeclarativeWebPage::grabWritten()
{
    QString path = m_grabWritter.result();
    emit grabResult(path);
}

void DeclarativeWebPage::thumbnailReady()
{
    QImage image = m_thumbnailResult->image();
    m_thumbnailResult.clear();
    int size = qMin(width(), height());
    QRect cropBounds(0, 0, size, size);

    image = image.copy(cropBounds);
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

QString DeclarativeWebPage::saveToFile(QImage image, QRect cropBounds)
{
    if (image.isNull()) {
        return "";
    }

    // 75% quality jpg produces small and good enough capture.
    QString path = QString("%1/tab-%2-thumb.jpg").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).arg(m_tabId);
    image = image.copy(cropBounds);
    return !allBlack(image) && image.save(path, "jpg", 75) ? path : "";
}

void DeclarativeWebPage::onRecvAsyncMessage(const QString& message, const QVariant& data)
{
    if (message == gFullScreenMessage) {
        setFullscreen(data.toMap().value(QString("fullscreen")).toBool());
    } else if (message == gDomContentLoadedMessage && data.toMap().value("rootFrame").toBool()) {
        m_domContentLoaded = true;
        emit domContentLoadedChanged();
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
        resetHeight();
        emit fullscreenChanged();
    }
}

QDebug operator<<(QDebug dbg, const DeclarativeWebPage *page)
{
    if (!page) {
        return dbg << "DeclarativeWebPage (this = 0x0)";
    }

    dbg.nospace() << "DeclarativeWebPage(url = " << page->url() << ", title = " << page->title() << ", width = " << page->width()
                  << ", height = " << page->height() << ", opacity = " << page->opacity()
                  << ", visible = " << page->isVisible() << ", enabled = " << page->isEnabled() << ")";
    return dbg.space();
}
