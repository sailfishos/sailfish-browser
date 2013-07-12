/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#ifndef SCREENGRABBER_H
#define SCREENGRABBER_H

#include <QObject>
#include <QStringList>

class QQuickView;

class ScreenGrabber : public QObject
{
    Q_OBJECT
public:
    static ScreenGrabber *instance();
    void screenCapture(QString url, int x, int y, int width, int height, qreal rotate);
    void setView(QQuickView *view);

signals:
    void screenCaptured(QString url, QString path);

private:
    explicit ScreenGrabber();
    ~ScreenGrabber();
    void saveToFile(QString url, QString path, QImage image, qreal rotate);

    QQuickView *m_view;
    QStringList paths;
};
#endif // SCREENGRABBER_H
