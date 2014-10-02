/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iconfetcher.h"

#include <QImage>

IconFetcher::IconFetcher(QObject *parent)
    : QObject(parent)
    , m_status(Null)
    , m_minimumIconSize(64) // Initial value that matches theme iconSizeMedium.
    , m_hasAcceptedTouchIcon(false)
{
}

void IconFetcher::fetch(const QString &iconUrl)
{
    updateAcceptedTouchIcon(false);
    QUrl url(iconUrl);
    QString path = url.path();
    updateStatus(Fetching);
    if (path.endsWith(".ico") || iconUrl.isEmpty()) {
        m_data = defaultIcon();
        updateStatus(Ready);
        emit dataChanged();
    } else {
        QNetworkRequest request(url);
        QNetworkReply *reply = m_networkAccessManager.get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(dataReady()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    }
}

IconFetcher::Status IconFetcher::status() const
{
    return m_status;
}

QString IconFetcher::data() const
{
    return m_data;
}

QString IconFetcher::defaultIcon() const
{
    return DEFAULT_DESKTOP_BOOKMARK_ICON;
}

bool IconFetcher::hasAcceptedTouchIcon()
{
    return m_hasAcceptedTouchIcon;
}

void IconFetcher::dataReady()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        QByteArray iconData = reply->readAll();
        QImage image;
        image.loadFromData(iconData);
        if (image.width() < m_minimumIconSize || image.height() < m_minimumIconSize) {
            m_data = defaultIcon();
        } else {
            m_data = QString(BASE64_IMAGE).arg(QString(iconData.toBase64()));
        }
        reply->deleteLater();

        updateAcceptedTouchIcon(true);
        updateStatus(Ready);
        emit dataChanged();
    } else {
        m_data = defaultIcon();
        updateStatus(Ready);
        emit dataChanged();
    }
}

void IconFetcher::error(QNetworkReply::NetworkError)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        reply->deleteLater();
    }

    m_data = defaultIcon();
    updateStatus(Error);
    emit dataChanged();
}

void IconFetcher::updateStatus(IconFetcher::Status status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

void IconFetcher::updateAcceptedTouchIcon(bool acceptedTouchIcon)
{
    if (m_hasAcceptedTouchIcon != acceptedTouchIcon) {
        m_hasAcceptedTouchIcon = acceptedTouchIcon;
        emit hasAcceptedTouchIconChanged();
    }
}
