/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativebookmarkmodel.h"
#include "bookmarkmanager.h"

DeclarativeBookmarkModel::DeclarativeBookmarkModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(BookmarkManager::instance(), &BookmarkManager::cleared,
            this, &DeclarativeBookmarkModel::clearBookmarks);
    bookmarks = BookmarkManager::instance()->load();

    // Generate mapping URL -> bookmark's index in the loaded list.
    int index(0);
    for (const Bookmark* const bookmark : bookmarks) {
        // Use multi insert as there might be multiple bookmark instances with the same url.
        bookmarkIndexes.insertMulti(bookmark->url(), index);
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

void DeclarativeBookmarkModel::add(const QString& url, const QString& title, const QString& favicon, bool touchIcon)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    bookmarkIndexes.insert(url, bookmarks.count());
    bookmarks.append(new Bookmark(title, url, favicon, touchIcon));
    endInsertRows();
    emit countChanged();
    // Getter will check if active page is still bookmarked.
    emit activeUrlBookmarkedChanged();

    save();
}

void DeclarativeBookmarkModel::remove(const QString& url)
{
    if (!contains(url)) {
        return;
    }

    int index = bookmarkIndexes.value(url, -1);
    remove(index);
}

void DeclarativeBookmarkModel::remove(int index)
{
    if (index >= 0 && index < bookmarks.count()) {
        beginRemoveRows(QModelIndex(), index, index);
        Bookmark* bookmark = bookmarks.takeAt(index);
        delete bookmark;

        // Remove index mapping and update remaining indices
        QMap<QString, int>::iterator i = bookmarkIndexes.begin();
        while (i != bookmarkIndexes.end()) {
            if (i.value() == index) {
                i = bookmarkIndexes.erase(i);
                // Erased item was the last one.
                if (i == bookmarkIndexes.end()) {
                    break;
                }
            }

            if (i.value() > index) {
                i.value()--;
            }
            i++;
        }

        endRemoveRows();

        emit countChanged();
        // Getter will check if active page is still bookmarked.
        emit activeUrlBookmarkedChanged();
        save();
    }
}

void DeclarativeBookmarkModel::updateFavoriteIcon(const QString &url, const QString &favicon, bool touchIcon)
{
    int bookmarkIndex = bookmarkIndexes.value(url, -1);
    if (bookmarkIndex >= 0) {
        Bookmark *bookmark = bookmarks[bookmarkIndex];
        QVector<int> roles;
        if (bookmark->favicon() != favicon) {
            roles << FaviconRole;
            bookmark->setFavicon(favicon);
        }
        if (bookmark->hasTouchIcon() != touchIcon) {
            roles << TouchIconRole;
            bookmark->setHasTouchIcon(touchIcon);
        }
        if (roles.count() > 0) {
            emit dataChanged(index(bookmarkIndex), index(bookmarkIndex), roles);
            save();
        }
    }
}

void DeclarativeBookmarkModel::edit(int index, const QString& url, const QString& title)
{
    if (index < 0 || index >= bookmarks.count())
        return;

    Bookmark * bookmark = bookmarks.value(index);
    QVector<int> roles;
    if (url != bookmark->url()) {
        bookmark->setUrl(url);
        roles << UrlRole;

        // Update key indexes
        QMap<QString, int>::iterator i = bookmarkIndexes.begin();
        while (i != bookmarkIndexes.end()) {
            if (i.value() == index) {
                i = bookmarkIndexes.erase(i);
                break;
            }
            ++i;
        }
        // Use multi insert here as the url might be already bookmarked.
        bookmarkIndexes.insertMulti(url, index);

        // Getter will check if active page is still bookmarked.
        emit activeUrlBookmarkedChanged();
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

QString DeclarativeBookmarkModel::activeUrl() const
{
    return m_activeUrl;
}

void DeclarativeBookmarkModel::setActiveUrl(const QString &url)
{
    if (m_activeUrl != url) {
        m_activeUrl = url;
        // Getter will check if active page is still bookmarked.
        emit activeUrlBookmarkedChanged();
        emit activeUrlChanged();
    }
}

bool DeclarativeBookmarkModel::activeUrlBookmarked() const
{
    return contains(m_activeUrl);
}

void DeclarativeBookmarkModel::clearBookmarks()
{
    beginRemoveRows(QModelIndex(), 0, qMax<int>(0, bookmarks.count()-1));
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
    if (index.row() < 0 || index.row() >= bookmarks.count())
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
