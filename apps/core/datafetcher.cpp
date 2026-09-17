/****************************************************************************
**
** Copyright (c) 2014 - 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "datafetcher.h"
#include "opensearchconfigs.h"
#include "faviconmanager.h"

#include <webengine.h>

#include <QBuffer>
#include <QImage>
#include <QUrl>
#include <QDir>
#include <QFile>

DataFetcher::DataFetcher(QObject *parent)
    : QObject(parent)
    , m_status(Null)
    , m_minimumIconSize(64) // Initial value that matches theme iconSizeMedium.
    , m_hasAcceptedTouchIcon(false)
    , m_type(Icon)
{
}

void DataFetcher::fetch(const QString &url)
{
    if (m_type == Icon)
        updateAcceptedTouchIcon(false);

    m_url = url;
    updateStatus(Fetching);
    if (m_type == Icon && url.isEmpty()) {
        m_data = defaultIcon();
        updateStatus(Ready);
        emit dataChanged();
    } else {
        m_networkData.clear();
        QNetworkRequest request(m_url);
        QNetworkReply *reply = m_networkAccessManager.get(request);
        connect(reply, &QNetworkReply::finished, this, &DataFetcher::dataReady);
        connect(reply,
                static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                this, &DataFetcher::error);
    }
}

DataFetcher::Status DataFetcher::status() const
{
    return m_status;
}

DataFetcher::Type DataFetcher::type() const
{
    return m_type;
}

void DataFetcher::setType(Type type)
{
    if (m_type != type) {
        m_type = type;
        emit typeChanged();
    }
}

QString DataFetcher::data() const
{
    return m_data;
}

QString DataFetcher::defaultIcon() const
{
    return FaviconManager::defaultDesktopBookmarkIcon();
}

bool DataFetcher::hasAcceptedTouchIcon()
{
    return m_hasAcceptedTouchIcon;
}

void DataFetcher::dataReady()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        m_networkData = reply->readAll();
        reply->deleteLater();
    }

    if (m_type == OpenSearch)
        saveAsSearchEngine();
    else
        saveAsImage();
}

void DataFetcher::saveAsImage()
{
    if (m_networkData.isEmpty()) {
        m_data = defaultIcon();
    } else {
        QImage image;
        image.loadFromData(m_networkData);
        if (image.width() < m_minimumIconSize || image.height() < m_minimumIconSize) {
            m_data = defaultIcon();
        } else {
            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            if (image.save(&buffer, "PNG")) {
                m_data = QStringLiteral("data:image/png;base64,")
                        + QString::fromLatin1(imageData.toBase64());
            } else {
                m_data = defaultIcon();
            }
        }
    }
    updateAcceptedTouchIcon(true);
    updateStatus(Ready);
    emit dataChanged();
}

void DataFetcher::error(QNetworkReply::NetworkError)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        disconnect(reply, &QNetworkReply::finished, this, &DataFetcher::dataReady);
        reply->deleteLater();
    }

    updateStatus(Error);
    if (m_type == Icon) {
        m_data = defaultIcon();
        emit dataChanged();
    }
}

void DataFetcher::updateStatus(DataFetcher::Status status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

void DataFetcher::updateAcceptedTouchIcon(bool acceptedTouchIcon)
{
    if (m_hasAcceptedTouchIcon != acceptedTouchIcon) {
        m_hasAcceptedTouchIcon = acceptedTouchIcon;
        emit hasAcceptedTouchIconChanged();
    }
}

void DataFetcher::saveAsSearchEngine()
{
    if (m_networkData.isEmpty()) {
        updateStatus(Error);
        return;
    }

    QUrl url = QUrl::fromLocalFile(OpenSearchConfigs::getOpenSearchConfigPath() + m_url.host() + ".xml");
    QDir dir;
    if (dir.mkpath(url.toString(QUrl::RemoveScheme | QUrl::RemoveFilename))) {
        QFile file(url.path());
        if (file.open(QIODevice::WriteOnly)) {
            if (file.write(m_networkData) > 0) {
                file.close();

                // Inform WebEngine there's a new search xml
                QVariantMap loadsearch;
                loadsearch.insert(QLatin1String("msg"), QVariant(QLatin1String("loadxml")));
                loadsearch.insert(QLatin1String("uri"), QVariant(url.toString()));
                SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("embedui:search"), QVariant(loadsearch));

                updateStatus(Ready);
            } else {
                file.close();
                updateStatus(Error);
            }
        }
    }
}
