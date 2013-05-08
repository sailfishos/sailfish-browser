/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#ifndef WEBTHUMBNAIL_H
#define WEBTHUMBNAIL_H

#include <QObject>
#include <QString>

class DeclarativeWebThumbnail : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString source READ source)

public:
    DeclarativeWebThumbnail(QString path ="", QObject *parent = 0);

    QString path();
    QString source();
    void setReady(bool ready);

signals:
    void pathChanged();
    
private:
    QString m_path;
    bool m_ready;
};

#endif // WEBTHUMBNAIL_H
