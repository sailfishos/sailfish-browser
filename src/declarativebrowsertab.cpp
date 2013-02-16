/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/


#include "declarativebrowsertab.h"
#include <QPixmap>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDebug>
#include <QFile>
#include <QTransform>


DeclarativeBrowserTab::DeclarativeBrowserTab(QDeclarativeView* view, QObject *parent) :
    m_view(view), QObject(parent)
{
    view->engine()->rootContext()->setContextProperty("BrowserTab",this);
}

DeclarativeBrowserTab::~DeclarativeBrowserTab() {
    for (int i = 0; i < paths.size(); ++i) {
        QFile f(paths.at(i));
        if (f.exists()) {
            f.remove();
        }
    }
    paths.clear();
}

QString DeclarativeBrowserTab::screenCapture(int x, int y, int width, int height) {
    QPixmap pixmap = QPixmap::grabWidget(m_view, x, y, width, height);

    int randomValue = abs(qrand());
    QString path = "/tmp/" + QString::number(randomValue);
    path.append(QString("-thumb.png"));
    pixmap.save(path);

    paths << path;
    return path;
}
