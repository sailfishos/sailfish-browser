/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/


#include "declarativebrowsertab.h"
#include <QScreen>
#include <QPixmap>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTransform>
#include <QStandardPaths>
#include <QFuture>
#include <QtConcurrentRun>
#include <QTime>
#include "declarativewebthumbnail.h"

DeclarativeBrowserTab::DeclarativeBrowserTab(QQuickView* view, QObject *parent) :
    QObject(parent), m_view(view)
{
    view->engine()->rootContext()->setContextProperty("BrowserTab",this);
    QTime now = QTime::currentTime();
    qsrand(now.msec());
}

DeclarativeBrowserTab::~DeclarativeBrowserTab()
{
    for (int i = 0; i < paths.size(); ++i) {
        QFile f(paths.at(i));
        if (f.exists()) {
            f.remove();
        }
    }
    paths.clear();
}

DeclarativeWebThumbnail* DeclarativeBrowserTab::screenCapture(int x, int y, int width, int height, qreal rotate)
{
    if(!m_view->isActive()) {
        return new DeclarativeWebThumbnail("");
    }
    QPixmap pixmap =  QGuiApplication::primaryScreen()->grabWindow(m_view->winId(), x, y, width, height);
    int randomValue = abs(qrand());
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" + QString::number(randomValue);
    path.append(QString("-thumb.png"));

    // asynchronous save to avoid the slow I/O
    DeclarativeWebThumbnail* thumb = new DeclarativeWebThumbnail(path);
    QtConcurrent::run(this, &DeclarativeBrowserTab::saveToFile, path, pixmap, rotate, thumb);
    return thumb;
}

void DeclarativeBrowserTab::saveToFile(QString path, QPixmap image, qreal rotate, DeclarativeWebThumbnail* thumb) {
    QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheLocation);
    if(!dir.exists()) {
        if(!dir.mkpath(cacheLocation)) {
            qWarning() << "Can't create directory "+ cacheLocation;
            return;
        }
    }
    QTransform transform;
    transform.rotate(rotate);
    image = image.transformed(transform);
    if(image.save(path)) {
        paths << path;
        thumb->setReady(true);
    }
}
