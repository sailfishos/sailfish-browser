/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEBOOKMARKMODEL_H
#define DECLARATIVEBOOKMARKMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QMap>

#include "bookmark.h"

class DeclarativeBookmarkModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    DeclarativeBookmarkModel(QObject *parent = 0);
    
    enum BookmarkRoles {
           UrlRole = Qt::UserRole + 1,
           TitleRole,
           FaviconRole,
           TouchIconRole,
    };

    Q_INVOKABLE void addBookmark(const QString& url, const QString& title, const QString& favicon, bool touchIcon = false);
    Q_INVOKABLE void removeBookmark(const QString& url);
    Q_INVOKABLE bool contains(const QString& url) const;
    Q_INVOKABLE void editBookmark(int index, const QString& url, const QString& title);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

private slots:
    void clearBookmarks();

signals:
    void countChanged();

private:
    void save();

    QList<Bookmark*> bookmarks;
    // This map accelerates access to the `bookmarks` list's elements by their URL.
    // Consider this as an analog of a DB index for `bookmarks` table indexed by URLs.
    QMap<QString, int> bookmarkIndexes;
};
#endif // DECLARATIVEBOOKMARKMODEL_H
