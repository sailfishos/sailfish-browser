/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include "screengrabber.h"
#include <QImage>
#include <QQuickView>
#include <QFile>
#include <QDir>
#include <QTransform>
#include <QStandardPaths>
#include <QtConcurrentRun>
#include <QTime>

ScreenGrabber::ScreenGrabber() :
    QObject(), m_view(0)
{
    QTime now = QTime::currentTime();
    qsrand(now.msec());
}

ScreenGrabber::~ScreenGrabber()
{
    for (int i = 0; i < paths.size(); ++i) {
        QFile f(paths.at(i));
        if (f.exists()) {
            f.remove();
        }
    }
    paths.clear();
}

void ScreenGrabber::screenCapture(QString url, int x, int y, int width, int height, qreal rotate)
{
    if (!m_view) {
        return;
    }
    if(!m_view->isActive()) {
        return;
    }
    QImage image = m_view->grabWindow();
    QImage cropped = image.copy(x, y, width, height);

    int randomValue = abs(qrand());
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" + QString::number(randomValue);
    path.append(QString("-thumb.png"));

    // asynchronous save to avoid the slow I/O
    QtConcurrent::run(this, &ScreenGrabber::saveToFile, url, path, cropped, rotate);
}

void ScreenGrabber::saveToFile(QString url, QString path, QImage image, qreal rotate) {
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
        emit screenCaptured(url, path);
        paths << path;
    } else {
        qWarning() << Q_FUNC_INFO << "failed to save image" << path;
    }
}

void ScreenGrabber::setView(QQuickView *view)
{
    m_view = view;
}

ScreenGrabber *ScreenGrabber::instance()
{
    static ScreenGrabber *browserTab;
    if (!browserTab) {
        browserTab = new ScreenGrabber();
    }
    return browserTab;
}
