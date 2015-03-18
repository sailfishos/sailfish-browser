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
#include "bookmarkmanager.h"

DeclarativeBookmarkModel::DeclarativeBookmarkModel(QObject *parent) :
    QAbstractListModel(parent)
{
    connect(BookmarkManager::instance(), SIGNAL(cleared()), this, SLOT(clearBookmarks()));
    bookmarks = BookmarkManager::instance()->load();

    // Generate mapping URL -> bookmark's index in the loaded list.
    int index(0);
    foreach (Bookmark* bookmark, bookmarks) {
        bookmarkIndexes.insert(bookmark->url(), index);
        index++;
    }
}

QHash<int, QByteArray> DeclarativeBookmarkModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[TitleRole] = "title";
    roles[FaviconRole] = "favicon";
    roles[TouchIconRole] = "hasTouchIcon";
    return roles;
}

void DeclarativeBookmarkModel::addBookmark(const QString& url, const QString& title, const QString& favicon, bool touchIcon)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    bookmarkIndexes.insert(url, bookmarks.count());
    bookmarks.append(new Bookmark(title, url, favicon, touchIcon));
    endInsertRows();

    emit countChanged();
    save();
}

void DeclarativeBookmarkModel::removeBookmark(const QString& url)
{
    if (!contains(url)) {
        return;
    }

    if (bookmarkIndexes.keys().contains(url)) {
        int index = bookmarkIndexes.value(url);
        beginRemoveRows(QModelIndex(), index, index);
        Bookmark* bookmark = bookmarks.takeAt(index);
        delete bookmark;
        bookmarkIndexes.remove(url);
        endRemoveRows();

        emit countChanged();
        save();
    }
}

void DeclarativeBookmarkModel::editBookmark(int index, const QString& url, const QString& title)
{
    if (index < 0 || index > bookmarks.count())
        return;

    Bookmark * bookmark = bookmarks.value(index);
    QVector<int> roles;
    if (url != bookmark->url()) {
        bookmark->setUrl(url);
        roles << UrlRole;
    }
    if (title != bookmark->title()) {
        bookmark->setTitle(title);
        roles << TitleRole;
    }
    if (roles.count() > 0) {
        QModelIndex modelIndex = QAbstractListModel::index(index);
        emit dataChanged(modelIndex, modelIndex, roles);
        save();
    }
}

void DeclarativeBookmarkModel::clearBookmarks()
{
    beginRemoveRows(QModelIndex(), 0, bookmarks.count()-1);
    bookmarks.clear();
    bookmarkIndexes.clear();
    endRemoveRows();
    emit countChanged();
}

void DeclarativeBookmarkModel::save()
{
    BookmarkManager::instance()->save(bookmarks);
}

int DeclarativeBookmarkModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    return bookmarks.count();
}

QVariant DeclarativeBookmarkModel::data(const QModelIndex & index, int role) const
{
    if (index.row() < 0 || index.row() > bookmarks.count())
        return QVariant();

    const Bookmark * bookmark = bookmarks.value(index.row());
    if (role == UrlRole) {
        return bookmark->url();
    } else if (role == TitleRole) {
        return bookmark->title();
    } else if (role == FaviconRole) {
        return bookmark->favicon();
    } else if (role == TouchIconRole) {
        return bookmark->hasTouchIcon();
    }
    return QVariant();
}

bool DeclarativeBookmarkModel::contains(const QString& url) const
{
    return bookmarkIndexes.contains(url);
}
