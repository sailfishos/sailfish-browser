/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativetab.h"

#include <QQuickWindow>
#include <QQuickView>
#include <QFile>
#include <QDir>
#include <QTransform>
#include <QStandardPaths>
#include <QtConcurrentRun>
#include <QTime>

#include "dbmanager.h"

DeclarativeTab::DeclarativeTab(QQuickItem *parent)
    : QQuickItem(parent)
    , m_tabId(0)
    , m_valid(false)
    , m_nextLinkId(0)
    , m_previousLinkId(0)
{
    init();
}

DeclarativeTab::~DeclarativeTab()
{
    m_screenCapturer.cancel();
    m_screenCapturer.waitForFinished();
}

void DeclarativeTab::init()
{
    connect(&m_screenCapturer, SIGNAL(finished()), this, SLOT(screenCaptureReady()));

    QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheLocation);
    if(!dir.exists()) {
        if(!dir.mkpath(cacheLocation)) {
            qWarning() << "Can't create directory "+ cacheLocation;
            return;
        }
    }
}

QString DeclarativeTab::thumbnailPath() const {
    return m_link.thumbPath();
}

void DeclarativeTab::setThumbnailPath(QString thumbPath) {
    if(thumbPath != m_link.thumbPath() && m_link.isValid()) {
        DBManager::instance()->updateThumbPath(m_link.url(), thumbPath, m_tabId);
    }
}

QString DeclarativeTab::url() const {
    return m_link.url();
}

void DeclarativeTab::setUrl(QString url)
{
    if (url != m_link.url() && m_link.isValid()) {
        updateTab(url, m_link.title());
    }
}

QString DeclarativeTab::title() const {
    return m_link.title();
}

void DeclarativeTab::setTitle(QString title) {
#ifdef DEBUG_LOGS
    qDebug() << title;
#endif
    if(title != m_link.title() && m_link.isValid()) {
        m_link.setTitle(title);
        emit titleChanged();
        DBManager::instance()->updateTitle(m_link.url(), title);
    }
}

int DeclarativeTab::tabId() const
{
    return m_tabId;
}

void DeclarativeTab::setTabId(int tabId) {
#ifdef DEBUG_LOGS
    qDebug() << m_tabId << " old values: " << m_link.title() << m_link.url() << " arg tab: " << tabId;
#endif

    if (tabId > 0 && tabId != m_tabId) {
        m_tabId = tabId;
    }

#ifdef DEBUG_LOGS
    qDebug() << "second condition:" << (tabId > 0) << m_valid;
#endif

    if ((tabId > 0) != m_valid) {
        m_valid = tabId > 0;
        emit validChanged();
    }
}

bool DeclarativeTab::valid() const
{
    return m_valid;
}

void DeclarativeTab::invalidate()
{
    m_tabId = 0;
    if (m_valid) {
        m_valid = false;
        emit validChanged();
    }

    if (!m_link.thumbPath().isEmpty()) {
        m_link.setThumbPath("");
        emit thumbPathChanged("", m_tabId);
    }

    if (!m_link.title().isEmpty()) {
        m_link.setTitle("");
        emit titleChanged();
    }

    if (!m_link.url().isEmpty()) {
        m_link.setUrl("");
        emit urlChanged();
    }

    if (m_nextLinkId != 0) {
        m_nextLinkId = 0;
        emit canGoFowardChanged();
    }

    if (m_previousLinkId != 0) {
        m_previousLinkId = 0;
        emit canGoBackChanged();
    }
}

bool DeclarativeTab::canGoForward() const {
    return m_nextLinkId > 0;
}

bool DeclarativeTab::canGoBack() const {
    return m_previousLinkId > 0;
}

void DeclarativeTab::goForward()
{
    if (m_nextLinkId > 0) {
        DBManager::instance()->goForward(m_tabId);
    }
}

void DeclarativeTab::goBack()
{
    if (m_previousLinkId > 0) {
        DBManager::instance()->goBack(m_tabId);
    }
}

void DeclarativeTab::navigateTo(QString url)
{
#ifdef DEBUG_LOGS
    qDebug() << "current link:" << m_link.url() << m_link.title() << "new url:" << url;
#endif
    if (url != m_link.url()) {
        m_link.setUrl(url);
        emit urlChanged();
        DBManager::instance()->navigateTo(m_tabId, url, m_link.title(), m_link.thumbPath());
    } else {
        updateTab(url, m_link.title());
    }
}

void DeclarativeTab::updateTab(QString url, QString title)
{
#ifdef DEBUG_LOGS
    qDebug() << title << url << m_tabId;
#endif

    bool urlHasChanged = false;
    bool titleHasChanged = false;

    if (url != m_link.url()) {
        urlHasChanged = true;
        m_link.setUrl(url);
        emit urlChanged();
    }

    if (title != m_link.title()) {
        titleHasChanged = true;
        m_link.setTitle(title);
        emit titleChanged();
    }

    if (urlHasChanged || titleHasChanged) {
        DBManager::instance()->updateTab(m_tabId, m_link.url(), m_link.title(), m_link.thumbPath());
    }
}

void DeclarativeTab::tabChanged(Tab tab)
{
#ifdef DEBUG_LOGS
    qDebug() << "old values:" << m_link.title() << m_link.url() << m_link.thumbPath() << "current tab:" <<  m_tabId << " changed tab:" << tab.tabId();
#endif
    bool thumbChanged = m_link.thumbPath() != tab.currentLink().thumbPath();
    bool titleStringChanged = m_link.title() != tab.currentLink().title();
    bool urlStringChanged = m_link.url() != tab.currentLink().url();

    setTabId(tab.tabId());
    m_link = tab.currentLink();
#ifdef DEBUG_LOGS
    qDebug() << "new values:" << m_link.title() << m_link.url() << m_link.thumbPath() << "current tab:" <<  m_tabId << " changed tab:" << tab.tabId();
    qDebug() << "previous link: " << m_previousLinkId << tab.previousLink() << m_nextLinkId << tab.nextLink();
#endif

    if (urlStringChanged) {
        emit urlChanged();
    }
    if (thumbChanged) {
        emit thumbPathChanged(m_link.thumbPath(), tab.tabId());
    }
    if (titleStringChanged) {
        emit titleChanged();
    }

    if (m_nextLinkId != tab.nextLink()) {
        m_nextLinkId = tab.nextLink();
        emit canGoFowardChanged();
    }

    if (m_previousLinkId != tab.previousLink()) {
        m_previousLinkId = tab.previousLink();
        emit canGoBackChanged();
    }

}

// Data changed in DB
void DeclarativeTab::updateThumbPath(QString url, QString path, int tabId)
{
    // TODO: Remove url parameter from this, worker, and manager.
    Q_UNUSED(url)
#ifdef DEBUG_LOGS
    qDebug() << url << m_link.url() << path << tabId;
#endif
    if (valid() && tabId == m_tabId && m_link.thumbPath() != path) {
        m_link.setThumbPath(path);
        emit thumbPathChanged(path, tabId);
    }
}

void DeclarativeTab::screenCaptureReady()
{
    ScreenCapture capture = m_screenCapturer.result();
#ifdef DEBUG_LOGS
    qDebug() << capture.tabId << capture.path << capture.url;
#endif
    if (capture.tabId != -1) {
        // Update immediately without dbworker round trip.
        updateThumbPath(capture.url, capture.path, capture.tabId);
        DBManager::instance()->updateThumbPath(capture.url, capture.path, capture.tabId);
    }
}

// Data changed in DB
void DeclarativeTab::updateTitle(QString url, QString title)
{
#ifdef DEBUG_LOGS
    qDebug() << url << title << m_tabId;
#endif
    if (m_link.url() == url) {
        if (title != m_link.title()) {
            m_link.setTitle(title);
            emit titleChanged();
        }
    }
}

/**
 * @brief DeclarativeTab::captureScreen
 * Rotation transformation is applied first, then geometry values on top of it.
 * @param url
 * @param x
 * @param y
 * @param width
 * @param height
 * @param rotate clockwise rotation of the image in degrees
 */
void DeclarativeTab::captureScreen(QString url, int x, int y, int width, int height, qreal rotate)
{
    if (!window() || !window()->isActive() || !valid()) {
        return;
    }

    // Cleanup old thumb.
    updateThumbPath(url, "", m_tabId);

    QImage image = window()->grabWindow();
    QRect cropBounds(x, y, width, height);

#ifdef DEBUG_LOGS
    qDebug() << "about to set future";
#endif
    // asynchronous save to avoid the slow I/O
    m_screenCapturer.setFuture(QtConcurrent::run(this, &DeclarativeTab::saveToFile, url, image, cropBounds, m_tabId, rotate));
}

DeclarativeTab::ScreenCapture DeclarativeTab::saveToFile(QString url, QImage image, QRect cropBounds, int tabId, qreal rotate) {
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
