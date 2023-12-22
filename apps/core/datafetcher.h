/****************************************************************************
**
** Copyright (c) 2014 - 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DATAFETCHER_H
#define DATAFETCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class DataFetcher : public QObject
{
    Q_OBJECT
    Q_ENUMS(Status)
    Q_ENUMS(Type)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString data READ data NOTIFY dataChanged FINAL)
    Q_PROPERTY(qreal minimumIconSize MEMBER m_minimumIconSize NOTIFY minimumIconSizeChanged)
    Q_PROPERTY(QString defaultIcon READ defaultIcon CONSTANT FINAL)
    Q_PROPERTY(bool hasAcceptedTouchIcon READ hasAcceptedTouchIcon NOTIFY hasAcceptedTouchIconChanged FINAL)
    Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)

public:
    enum Status { Null, Fetching, Ready, Error };
    enum Type { Icon, OpenSearch };

    explicit DataFetcher(QObject *parent = 0);

    Q_INVOKABLE void fetch(const QString &iconUrl);

    Status status() const;
    QString data() const;
    QString defaultIcon() const;
    bool hasAcceptedTouchIcon();
    Type type() const;
    void setType(Type type);

signals:
    void statusChanged();
    void dataChanged();
    void minimumIconSizeChanged();
    void hasAcceptedTouchIconChanged();
    void typeChanged();

private slots:
    void dataReady();
    void error(QNetworkReply::NetworkError);

private:
    void updateStatus(Status status);
    void updateAcceptedTouchIcon(bool acceptedTouchIcon);
    void saveAsImage();
    void saveAsSearchEngine();

    QNetworkAccessManager m_networkAccessManager;
    Status m_status;
    QString m_data;
    qreal m_minimumIconSize;
    bool m_hasAcceptedTouchIcon;
    QByteArray m_networkData;
    QUrl m_url;
    Type m_type;
};


#endif // DATAFETCHER_H
