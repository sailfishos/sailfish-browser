/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <memory>

#include "browserpaths.h"
#include "datafetcher.h"
#include "logging.h"

#include "faviconmanager.h"

#include "declarativewebpage.h"

FaviconManager::FaviconManager(QObject *parent)
    : QObject(parent)
    , m_faviconSets()
{
}

FaviconManager *FaviconManager::instance()
{
    static FaviconManager *singleton = nullptr;
    if (!singleton) {
        singleton = new FaviconManager();
    }

    return singleton;
}

QString FaviconManager::sanitizedHostname(const QString &hostname)
{
    // Should port should be included too?
    const QUrl url(hostname);
    return QStringLiteral("%1://%2").arg(url.scheme(), url.host());
}

void FaviconManager::save(const QString &type)
{
    if (!m_faviconSets.contains(type)) {
        qCDebug(lcFavoritesLog) << "No" << type << "favicons loaded, skipping save";
        return;
    }

    const FaviconSet &faviconSet = m_faviconSets.value(type);
    if (!faviconSet.loaded) {
        qCDebug(lcFavoritesLog) << "No changes to" << type << "favicons, skipping save";
        return;
    }

    QString dataLocation = BrowserPaths::dataLocation();
    if (dataLocation.isNull()) {
        qWarning() << "No datalocation set to save" << type << "favicons to";
        return;
    }
    QString path = QString("%1/%2.json").arg(dataLocation).arg(type);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Can't create favicons file " << path;
        return;
    }
    QTextStream out(&file);
    QJsonArray items;

    for (const QString &hostname : faviconSet.favicons.keys()) {
        const Favicon &favicon = faviconSet.favicons.value(hostname);
        QJsonObject item;
        item.insert("hostname", QJsonValue(hostname));
        item.insert("favicon", QJsonValue(favicon.favicon));
        item.insert("hasTouchIcon", QJsonValue(favicon.hasTouchIcon));
        items.append(QJsonValue(item));
    }
    QJsonDocument doc(items);
    out.setCodec("UTF-8");
    out << doc.toJson();
    file.close();
}

// After calling load it must be safe to assume the type exists in the map
void FaviconManager::load(const QString &type)
{
    // Once loaded, future calls to load return immediately
    if (m_faviconSets.contains(type) && m_faviconSets.value(type).loaded) {
        // Favicons already loaded
        return;
    }

    FaviconSet faviconSet;
    faviconSet.loaded = true;
    m_faviconSets.insert(type, faviconSet);

    QString dataLocation = BrowserPaths::dataLocation();
    if (dataLocation.isNull()) {
        qWarning() << "No datalocation set to load" << type << "favicons from";
        return;
    }
    QString path = QString("%1/%2.json").arg(dataLocation).arg(type);
    QScopedPointer<QFile> file(new QFile(path));

    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        // The file may not exist yet; that's okay
        qCDebug(lcFavoritesLog) << "Unable to open favicons file " << path;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file->readAll());
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            if (value.isObject()) {
                QJsonObject obj = value.toObject();
                QString hostname = obj.value("hostname").toString();
                Favicon favicon;
                favicon.favicon = obj.value("favicon").toString();
                favicon.hasTouchIcon = obj.value("hasTouchIcon").toBool();
                faviconSet.favicons.insert(hostname, favicon);
            }
        }
    } else {
        qWarning() << "Favicons json file should be an array of items";
    }
    file->close();

    m_faviconSets.insert(type, faviconSet);
}

void FaviconManager::add(const QString &type, const QString &hostname, const QString &favicon, bool hasTouchIcon)
{
    load(type);

    const QString host = sanitizedHostname(hostname);
    FaviconSet faviconSet = m_faviconSets.value(type);

    if (faviconSet.favicons.contains(host)) {
        const Favicon &current = faviconSet.favicons.value(host);
        if ((current.favicon == favicon) && (current.hasTouchIcon == hasTouchIcon)) {
            // No changes
            return;
        }
    }

    Favicon item;
    item.favicon = favicon;
    item.hasTouchIcon = hasTouchIcon;
    // After calling load() it's safe to assume the type exists in the map
    faviconSet.favicons.insert(host, item);
    m_faviconSets.insert(type, faviconSet);

    save(type);
}

void FaviconManager::remove(const QString &type, const QString &hostname)
{
    load(type);

    // After calling load() it's safe to assume the type exists in the map
    FaviconSet faviconSet = m_faviconSets.value(type);
    faviconSet.favicons.remove(sanitizedHostname(hostname));
    m_faviconSets.insert(type, faviconSet);

    save(type);
}

QString FaviconManager::get(const QString &type, const QString &hostname)
{
    load(type);

    // After calling load() it's safe to assume the type exists in the map
    QString favicon;
    QString host = sanitizedHostname(hostname);
    if (m_faviconSets.value(type).favicons.contains(host)) {
        favicon = m_faviconSets.value(type).favicons.value(host).favicon;
    }
    return favicon;
}

void FaviconManager::grabIcon(const QString &type, DeclarativeWebPage *webPage, const QSize &size)
{
    if (!get(type, webPage->url().toString()).isEmpty()) {
        return; // favicon was previously already loaded.
    }

    DataFetcher *dataFetcher = new DataFetcher(this);

    std::shared_ptr<QMetaObject::Connection> dataConn = std::make_shared<QMetaObject::Connection>();
    *dataConn = connect(dataFetcher, &DataFetcher::dataChanged, this, [this, dataFetcher, type, webPage, size, dataConn]() {
        QObject::disconnect(*dataConn);
        if (dataFetcher->hasAcceptedTouchIcon()) {
            qCDebug(lcFavoritesLog) << "Storing favicon for" << type;
            add(type, webPage->url().toString(), dataFetcher->data(), true);
        }
        else {
            std::shared_ptr<QMetaObject::Connection> thumbConn = std::make_shared<QMetaObject::Connection>();
            *thumbConn = connect(webPage, &DeclarativeWebPage::thumbnailResult, [this, type, webPage, thumbConn](const QString &data) {
                qCDebug(lcFavoritesLog) << "Storing thumbnail for" << type;
                QObject::disconnect(*thumbConn);
                add(type, webPage->url().toString(), data, false);
            });
            webPage->grabThumbnail(size);
        }
        dataFetcher->deleteLater();
    });
    dataFetcher->fetch(webPage->property("favicon").toString());
}

void FaviconManager::clear(const QString &type)
{
    FaviconSet faviconSet;
    faviconSet.loaded = true;
    m_faviconSets.insert(type, faviconSet);
    save(type);
}
