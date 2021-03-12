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
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <MGConfItem>

#include "bookmark.h"
#include "browserpaths.h"

BookmarkManager::BookmarkManager()
  : QObject(nullptr)
{
    m_clearBookmarksConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_bookmarks", this);

    clearBookmarks();

    connect(m_clearBookmarksConfItem.data(), &MGConfItem::valueChanged,
            this, &BookmarkManager::clearBookmarks);
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
    QString dataLocation = BrowserPaths::dataLocation();
    if (dataLocation.isNull()) {
        return;
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
    QString bookmarkFile = BrowserPaths::dataLocation() + "/bookmarks.json";
    QScopedPointer<QFile> file(new QFile(bookmarkFile));

    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open bookmarks " << bookmarkFile;

        file.reset(new QFile(QLatin1Literal("/usr/share/sailfish-browser/default-content/bookmarks.json")));
        if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Unable to open bookmarks defaults";
            return bookmarks;
        }
    }

    QJsonDocument doc = QJsonDocument::fromJson(file->readAll());
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            if (value.isObject()) {
                QJsonObject obj = value.toObject();
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

void BookmarkManager::clearBookmarks()
{
    if (m_clearBookmarksConfItem->value(false).toBool()) {
        clear();
        m_clearBookmarksConfItem->set(false);
    }
}
