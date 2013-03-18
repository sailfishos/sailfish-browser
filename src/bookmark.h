/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <QObject>

class Bookmark : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString favicon READ favicon WRITE setFavicon NOTIFY faviconChanged)

public:
    Bookmark(QString title, QString url, QString favicon, QObject* parent = 0);

    QString title() const;
    void setTitle(QString title);

    QString url() const;
    void setUrl(QString url);

    QString favicon() const;
    void setFavicon(QString favicon);

signals:
    void titleChanged();
    void urlChanged();
    void faviconChanged();

private:
    QString m_title;
    QString m_url;
    QString m_favicon;
};

#endif // BOOKMARK_H
