/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

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

}

void DeclarativeTab::init()
{
    connect(DBManager::instance(), SIGNAL(tabChanged(Tab)),
            this, SLOT(tabChanged(Tab)));
    connect(DBManager::instance(), SIGNAL(tabAvailable(Tab)),
            this, SLOT(tabChanged(Tab)));
    connect(DBManager::instance(), SIGNAL(thumbPathChanged(QString,QString,int)),
            this, SLOT(updateThumbPath(QString,QString,int)));
    connect(DBManager::instance(), SIGNAL(titleChanged(QString,QString)),
            this, SLOT(updateTitle(QString,QString)));

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

QString DeclarativeTab::title() const {
    return m_link.title();
}

void DeclarativeTab::setTitle(QString title) {
#ifdef DEBUG_LOGS
    qDebug() << title;
#endif
    if(title != m_link.title() && m_link.isValid()) {
        DBManager::instance()->updateTitle(m_link.url(), title);
    }
}

int DeclarativeTab::tabId() const {
    return m_tabId;
}

void DeclarativeTab::setTabId(int tabId) {
#ifdef DEBUG_LOGS
    qDebug() << m_tabId << " old values: " << m_link.title() << m_link.url() << " arg tab: " << tabId;
#endif

    if (tabId > 0 && tabId != m_tabId) {
        m_tabId = tabId;
        DBManager::instance()->getTab(m_tabId);
        emit tabIdChanged();
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

void DeclarativeTab::navigateTo(QString url, QString title, QString path)
{
#ifdef DEBUG_LOGS
    qDebug() << m_link.url() << m_link.title() << title << url << path;
#endif
    if (url != m_link.url()) {
        DBManager::instance()->navigateTo(m_tabId, url, title, path);
    } else {
        updateTab(url, title, path);
    }
}

void DeclarativeTab::updateTab(QString url, QString title, QString path)
{
#ifdef DEBUG_LOGS
    qDebug() << title << url << m_tabId << path;
#endif
    if ((!url.isEmpty() && url != m_link.url())
            || (!title.isEmpty() && title != m_link.title())
            || (!path.isEmpty() && path != m_link.title())) {
        DBManager::instance()->updateTab(m_tabId, url, title, path);
    }
}

// Data changed in DB
void DeclarativeTab::tabChanged(Tab tab)
{
    // For valid tab "tab.tabId() != m_tabId" is ok as request is set by
    // setTabId. For invalid tab we can reset all properties.
    if (tab.tabId() != m_tabId && tab.isValid()) {
        return;
    }

#ifdef DEBUG_LOGS
    qDebug() << "old values:" << m_link.title() << m_link.url() << "current tab:" <<  m_tabId << " changed tab:" << tab.tabId();
#endif
    bool thumbChanged = m_link.thumbPath() != tab.currentLink().thumbPath();
    bool titleStringChanged = m_link.title() != tab.currentLink().title();
    bool urlStringChanged = m_link.url() != tab.currentLink().url();

    m_link = tab.currentLink();
#ifdef DEBUG_LOGS
    qDebug() << "new values:" << m_link.title() << m_link.url() << "current tab:" <<  m_tabId << " changed tab:" << tab.tabId();
    qDebug() << "previous link: " << m_previousLinkId << tab.previousLink() << m_nextLinkId << tab.nextLink();
#endif

    if (urlStringChanged) {
        emit urlChanged();
    }
    if (thumbChanged) {
        emit thumbPathChanged();
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
    Q_UNUSED(url)
#ifdef DEBUG_LOGS
    qDebug() << url << m_link.url() << path << tabId;
#endif
    if (valid() && tabId == m_tabId) {
        m_link.setThumbPath(path);
        emit thumbPathChanged();
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

    QImage image = window()->grabWindow();
    QRect cropBounds(x, y, width, height);

    // asynchronous save to avoid the slow I/O
    QtConcurrent::run(this, &DeclarativeTab::saveToFile, url, image, cropBounds, m_tabId, rotate);
}

void DeclarativeTab::saveToFile(QString url, QImage image, QRect cropBounds, int tabId, qreal rotate) {
    QString path = QString("%1/tab-%2-thumb.jpg").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).arg(tabId);
    QTransform transform;
    transform.rotate(360 - rotate);
    image = image.transformed(transform);
    image = image.copy(cropBounds);

    if(image.save(path)) {
        DBManager::instance()->updateThumbPath(url, path, tabId);
    } else {
        qWarning() << Q_FUNC_INFO << "failed to save image" << path;
    }
}
