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

class DeclarativeBrowserTab : public QObject
{
    Q_OBJECT
public:
    explicit DeclarativeBrowserTab(QDeclarativeView* view, QObject *parent = 0);
    ~DeclarativeBrowserTab();

    Q_INVOKABLE QString screenCapture(int x, int y, int width, int height);
    
private:
    QDeclarativeView * m_view;
    QStringList paths;
};
#endif // DECLARATIVEBROWSERTAB_H
