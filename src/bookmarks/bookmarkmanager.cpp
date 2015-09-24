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

void BookmarkManager::save(const QList<Bookmark*> & bookmarks)
{
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(dataLocation);
    if (!dir.exists()) {
        if (!dir.mkpath(dataLocation)) {
            qWarning() << "Can't create directory " << dataLocation;
            return;
        }
    }
    QString path = dataLocation + "/bookmarks.json";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Can't create file " << path;
        return;
    }
    QTextStream out(&file);
    QJsonArray items;

    foreach (Bookmark* bookmark, bookmarks) {
        QJsonObject title;
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
    save(QList<Bookmark*>());
    emit cleared();
}

QList<Bookmark*> BookmarkManager::load() {
    QList<Bookmark*> bookmarks;
    QString bookmarkFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/bookmarks.json";
    QScopedPointer<QFile> file(new QFile(bookmarkFile));

    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open bookmarks " << bookmarkFile;
#ifdef TEST_DATA
        file.reset(new QFile(QString("%1%2").arg(TEST_DATA, QLatin1Literal("/bookmarks.json"))));
#else
        file.reset(new QFile(QLatin1Literal("/usr/share/sailfish-browser/default-content/bookmarks.json")));
#endif
        if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Unable to open bookmarks defaults";
            return bookmarks;
        }
    }

    QJsonDocument doc = QJsonDocument::fromJson(file->readAll());
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        QJsonArray::iterator i;
        for (i=array.begin(); i != array.end(); ++i) {
            if ((*i).isObject()) {
                QJsonObject obj = (*i).toObject();
                QString url = obj.value("url").toString();
                QString favicon = obj.value("favicon").toString();
                Bookmark* m = new Bookmark(obj.value("title").toString(),
                                           url,
                                           favicon,
                                           obj.value("hasTouchIcon").toBool());
                bookmarks.append(m);
            }
        }
    } else {
        qWarning() << "Bookmarks.json should be an array of items";
    }
    file->close();
    return bookmarks;
}
