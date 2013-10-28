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
    qDebug() << "DeclarativeTab::setTitle:" << title;
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
    qDebug() << "DeclarativeTab::setTabId:" << m_tabId << " old values: " << m_link.title() << m_link.url();
#endif

    if (tabId > 0 && tabId != m_tabId) {
        m_tabId = tabId;
        DBManager::instance()->getTab(m_tabId);
        emit tabIdChanged();
    }

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
    if (url != m_link.url()) {
#ifdef DEBUG_LOGS
        qDebug() << "DeclarativeTab::navigateTo:" << title << url;
#endif
        DBManager::instance()->navigateTo(m_tabId, url, title, path);
    } else {
        updateTab(url, title, path);
    }
}

void DeclarativeTab::updateTab(QString url, QString title, QString path)
{
#ifdef DEBUG_LOGS
    qDebug() << "DeclarativeTab::updateTab:" << title << url << m_tabId << path;
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
    qDebug() << "DeclarativeTab::tabChanged old values:" << m_link.title() << m_link.url() << "current tab:" <<  m_tabId << " changed tab:" << tab.tabId();
#endif
    bool thumbChanged = m_link.thumbPath() != tab.currentLink().thumbPath();
    bool titleStringChanged = m_link.title() != tab.currentLink().title();
    bool urlStringChanged = m_link.url() != tab.currentLink().url();

    m_link = tab.currentLink();
#ifdef DEBUG_LOGS
    qDebug() << "DeclarativeTab::tabChanged new values:" << m_link.title() << m_link.url() << "current tab:" <<  m_tabId << " changed tab:" << tab.tabId();
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
    qDebug() << "DeclarativeTab::updateThumbPath:" << url << m_link.url() << path << tabId;
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
    qDebug() << "DeclarativeTab::updateTitle:" << url << title << m_tabId;
#endif
    if (m_link.url() == url) {
        if (title != m_link.title()) {
            m_link.setTitle(title);
            emit titleChanged();
        }
    }
}

void DeclarativeTab::captureScreen(QString url, int x, int y, int width, int height, qreal rotate)
{
    if (!window() || !window()->isActive() || !valid()) {
        return;
    }

    QImage image = window()->grabWindow();
    // TODO: Cropping should be done in QtConcurrent / thread
    QImage cropped = image.copy(x, y, width, height);
    QString path = QString("%1/tab-%2-thumb.jpg").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).arg(m_tabId);
    // asynchronous save to avoid the slow I/O
    QtConcurrent::run(this, &DeclarativeTab::saveToFile, url, path, cropped, m_tabId, rotate);
}

void DeclarativeTab::saveToFile(QString url, QString path, QImage image, int tabId, qreal rotate) {
    QTransform transform;
    transform.rotate(rotate);

    image = image.transformed(transform);
    if(image.save(path)) {
        DBManager::instance()->updateThumbPath(url, path, tabId);
    } else {
        qWarning() << Q_FUNC_INFO << "failed to save image" << path;
    }
}
