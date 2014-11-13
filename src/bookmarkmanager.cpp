/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bookmarkmanager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

#include "bookmark.h"

BookmarkManager::BookmarkManager()
{
}

BookmarkManager* BookmarkManager::instance()
{
    static BookmarkManager* singleton;
    if (!singleton) {
        singleton = new BookmarkManager();
    }

    return singleton;
}

void BookmarkManager::save(const QMap<QString, Bookmark*> & bookmarks)
{
    QString settingsLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(settingsLocation);
    if (!dir.exists()) {
        if (!dir.mkpath(settingsLocation)) {
            qWarning() << "Can't create directory " << settingsLocation;
            return;
        }
    }
    QString path = settingsLocation + "/bookmarks.json";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Can't create file " << path;
        return;
    }
    QTextStream out(&file);
    QJsonArray items;

    QMapIterator<QString, Bookmark*> bookmarkIterator(bookmarks);
    while (bookmarkIterator.hasNext()) {
        bookmarkIterator.next();
        QJsonObject title;
        Bookmark* bookmark = bookmarkIterator.value();
        title.insert("url", QJsonValue(bookmark->url()));
        title.insert("title", QJsonValue(bookmark->title()));
        title.insert("favicon", QJsonValue(bookmark->favicon()));
        title.insert("hasTouchIcon", QJsonValue(bookmark->hasTouchIcon()));
        items.append(QJsonValue(title));
    }
    QJsonDocument doc(items);
    out.setCodec("UTF-8");
    out << doc.toJson();
    file.close();
}

void BookmarkManager::clear()
{
    save(QMap<QString, Bookmark*>());
    emit cleared();
}

QMap<QString, Bookmark*> BookmarkManager::load() {
    QMap<QString, Bookmark*> bookmarks;
    QString settingsLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/bookmarks.json";
    QScopedPointer<QFile> file(new QFile(settingsLocation));

    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open bookmarks " << settingsLocation;

        file.reset(new QFile(QLatin1Literal("/usr/share/sailfish-browser/content/bookmarks.json")));
        if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Unable to open bookmarks defaults";
            return bookmarks;
        }
    }

    QJsonDocument doc = QJsonDocument::fromJson(file->readAll());
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        QJsonArray::iterator i;
        QRegularExpression jollaUrl("^http[s]?://(together.)?jolla.com");
        for (i=array.begin(); i != array.end(); ++i) {
            if ((*i).isObject()) {
                QJsonObject obj = (*i).toObject();
                QString url = obj.value("url").toString();
                QString favicon = obj.value("favicon").toString();
                if (url.contains(jollaUrl) ||
                        url.startsWith("http://m.youtube.com/playlist?list=PLQgR2jhO_J0y8YSSvVd-Mg9LM88W0aIpD")) {
                    favicon = "image://theme/icon-m-service-jolla";
                }

                Bookmark* m = new Bookmark(obj.value("title").toString(),
                                           url,
                                           favicon,
                                           obj.value("hasTouchIcon").toBool());
                bookmarks.insert(url, m);
            }
        }
    } else {
        qWarning() << "Bookmarks.json should be an array of items";
    }
    file->close();
    return bookmarks;
}
