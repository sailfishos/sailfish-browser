/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativebookmarkmodel.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QTextStream>

DeclarativeBookmarkModel::DeclarativeBookmarkModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

QHash<int, QByteArray> DeclarativeBookmarkModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[TitleRole] = "title";
    roles[FaviconRole] = "favicon";
    return roles;
}

// TODO cleanup
void DeclarativeBookmarkModel::addBookmark(const QString& url, const QString& title, const QString& favicon) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    bookmarks.insert(url, new Bookmark(title, url, favicon));
    bookmarkUrls.append(url);
    endInsertRows();

    emit countChanged();
    save();
}

void DeclarativeBookmarkModel::removeBookmark(const QString& url) {
    if (!contains(url)) {
        return;
    }

    if (bookmarks.contains(url)) {
        int index = bookmarkUrls.indexOf(url);
        beginRemoveRows(QModelIndex(), index, index);
        Bookmark* bookmark = bookmarks.take(url);
        delete bookmark;
        bookmarkUrls.removeAt(index);
        endRemoveRows();

        emit countChanged();
        save();
    }
}

void DeclarativeBookmarkModel::componentComplete() {
    QString settingsLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/bookmarks.json";
    QScopedPointer<QFile> file(new QFile(settingsLocation));

    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open bookmarks "+settingsLocation;

        file.reset(new QFile(QLatin1Literal("/usr/share/sailfish-browser/content/bookmarks.json")));
        if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Unable to open bookmarks defaults";
            return;
        }
    }

    QJsonDocument doc = QJsonDocument::fromJson(file->readAll());
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        QJsonArray::iterator i;
        for(i=array.begin(); i != array.end(); ++i) {
            if((*i).isObject()) {
                QJsonObject obj = (*i).toObject();
                QString url = obj.value("url").toString();
                Bookmark* m = new Bookmark(obj.value("title").toString(),
                                           url,
                                           obj.value("favicon").toString());
                bookmarks.insert(url, m);
                bookmarkUrls.append(url);
            }
        }
    } else {
        qWarning() << "Bookmarks.json should be an array of items";
    }
    emit countChanged();
    file->close();
}

void DeclarativeBookmarkModel::classBegin() {}

void DeclarativeBookmarkModel::save() {
    QString settingsLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(settingsLocation);
    if(!dir.exists()) {
        if(!dir.mkpath(settingsLocation)) {
            qWarning() << "Can't create directory "+ settingsLocation;
            return;
        }
    }
    QString path = settingsLocation + "/bookmarks.json";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Can't create file "+ path;
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
        items.append(QJsonValue(title));
    }
    QJsonDocument doc(items);
    out.setCodec("UTF-8");
    out << doc.toJson();
    file.close();
}

int DeclarativeBookmarkModel::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent)
    return bookmarks.count();
}

QVariant DeclarativeBookmarkModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() > bookmarkUrls.count())
        return QVariant();

    const Bookmark * bookmark = bookmarks.value(bookmarkUrls[index.row()]);
    if (role == UrlRole) {
        return bookmark->url();
    } else if (role == TitleRole) {
        return bookmark->title();
    } else if (role == FaviconRole) {
        return bookmark->favicon();
    }
    return QVariant();
}

bool DeclarativeBookmarkModel::contains(const QString& url) const {
    return bookmarks.contains(url);
}
