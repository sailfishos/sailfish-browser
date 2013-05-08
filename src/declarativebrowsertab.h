/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#ifndef DECLARATIVEBROWSERTAB_H
#define DECLARATIVEBROWSERTAB_H

#include <QObject>
#include <QDeclarativeView>
#include <QStringList>
#include <QPixmap>
#include "declarativewebthumbnail.h"

class DeclarativeBrowserTab : public QObject
{
    Q_OBJECT
public:
    explicit DeclarativeBrowserTab(QDeclarativeView* view, QObject *parent = 0);
    ~DeclarativeBrowserTab();

    Q_INVOKABLE DeclarativeWebThumbnail* screenCapture(int x, int y, int width, int height, qreal rotate);
    
private:
    void saveToFile(QString path, QPixmap image, qreal rotate, DeclarativeWebThumbnail* thumb);

    QDeclarativeView * m_view;
    QStringList paths;
};
#endif // DECLARATIVEBROWSERTAB_H
