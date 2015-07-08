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
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(QString activeUrl READ activeUrl WRITE setActiveUrl NOTIFY activeUrlChanged FINAL)
    Q_PROPERTY(bool activeUrlBookmarked READ activeUrlBookmarked NOTIFY activeUrlBookmarkedChanged FINAL)
public:
    DeclarativeBookmarkModel(QObject *parent = 0);
    
    enum BookmarkRoles {
           UrlRole = Qt::UserRole + 1,
           TitleRole,
           FaviconRole,
           TouchIconRole,
    };

    Q_INVOKABLE void add(const QString& url, const QString& title, const QString& favicon, bool touchIcon = false);
    Q_INVOKABLE void remove(const QString& url);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void updateFavoriteIcon(const QString& url, const QString& favicon, bool touchIcon);
    Q_INVOKABLE bool contains(const QString& url) const;
    Q_INVOKABLE void edit(int index, const QString& url, const QString& title);

    QString activeUrl() const;
    void setActiveUrl(const QString& url);

    bool activeUrlBookmarked() const;

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

private slots:
    void clearBookmarks();

signals:
    void countChanged();
    void activeUrlChanged();
    void activeUrlBookmarkedChanged();

private:
    void save();

    QString m_activeUrl;

    QList<Bookmark*> bookmarks;
    // This map accelerates access to the `bookmarks` list's elements by their URL.
    // Consider this as an analog of a DB index for `bookmarks` table indexed by URLs.
    QMap<QString, int> bookmarkIndexes;
};
#endif // DECLARATIVEBOOKMARKMODEL_H
