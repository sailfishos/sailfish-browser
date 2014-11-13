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
    bookmarkUrls = bookmarks.keys();
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
    bookmarks.insert(url, new Bookmark(title, url, favicon, touchIcon));
    bookmarkUrls.append(url);
    endInsertRows();

    emit countChanged();
    save();
}

void DeclarativeBookmarkModel::removeBookmark(const QString& url)
{
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

void DeclarativeBookmarkModel::editBookmark(int index, const QString& url, const QString& title)
{
    if (index < 0 || index > bookmarkUrls.count())
        return;

    Bookmark * bookmark = bookmarks.value(bookmarkUrls[index]);
    QVector<int> roles;
    if (url != bookmark->url()) {
        bookmark->setUrl(url);
        bookmarks.remove(bookmarkUrls[index]);
        bookmarks.insert(url, bookmark);
        bookmarkUrls[index] = url;
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
    beginRemoveRows(QModelIndex(), 0, bookmarkUrls.count()-1);
    bookmarks.clear();
    bookmarkUrls.clear();
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
    if (index.row() < 0 || index.row() > bookmarkUrls.count())
        return QVariant();

    const Bookmark * bookmark = bookmarks.value(bookmarkUrls[index.row()]);
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
    return bookmarks.contains(url);
}
