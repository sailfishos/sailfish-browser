/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ICONFETCHER_H
#define ICONFETCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class IconFetcher : public QObject {
    Q_OBJECT
    Q_ENUMS(Status)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString data READ data NOTIFY dataChanged FINAL)
    Q_PROPERTY(qreal minimumIconSize MEMBER m_minimumIconSize NOTIFY minimumIconSizeChanged)
    Q_PROPERTY(QString defaultIcon READ defaultIcon CONSTANT FINAL)
    Q_PROPERTY(bool hasAcceptedTouchIcon READ hasAcceptedTouchIcon NOTIFY hasAcceptedTouchIconChanged FINAL)

public:
    enum Status { Null, Fetching, Ready, Error };

    explicit IconFetcher(QObject *parent = 0);

    Q_INVOKABLE void fetch(const QString &iconUrl);

    Status status() const;
    QString data() const;
    QString defaultIcon() const;
    bool hasAcceptedTouchIcon();

signals:
    void statusChanged();
    void dataChanged();
    void minimumIconSizeChanged();
    void hasAcceptedTouchIconChanged();

private slots:
    void dataReady();
    void error(QNetworkReply::NetworkError);

private:
    void updateStatus(Status status);
    void updateAcceptedTouchIcon(bool acceptedTouchIcon);

    QNetworkAccessManager m_networkAccessManager;
    Status m_status;
    QString m_data;
    qreal m_minimumIconSize;
    bool m_hasAcceptedTouchIcon;
};


#endif // ICONFETCHER_H
