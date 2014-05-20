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
#include <QQmlParserStatus>

#include "bookmark.h"

class DeclarativeBookmarkModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    DeclarativeBookmarkModel(QObject *parent = 0);
    
    enum BookmarkRoles {
           UrlRole = Qt::UserRole + 1,
           TitleRole,
           FaviconRole
    };

    Q_INVOKABLE void addBookmark(const QString& url, const QString& title, const QString& favicon);
    Q_INVOKABLE void removeBookmark(const QString& url);
    Q_INVOKABLE bool contains(const QString& url) const;
    Q_INVOKABLE void editBookmark(int index, const QString& url, const QString& title);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    // From QQmlParserStatus
    void classBegin();
    void componentComplete();

signals:
    void countChanged();

private:
    void save();

    QMap<QString, Bookmark*> bookmarks;
    QStringList bookmarkUrls;
};
#endif // DECLARATIVEBOOKMARKMODEL_H
