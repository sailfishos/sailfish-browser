/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#ifndef DECLARATIVEBOOKMARKMODEL_H
#define DECLARATIVEBOOKMARKMODEL_H

#include <QAbstractListModel>
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
           FaviconRole
    };

    Q_INVOKABLE void addBookmark(const QString& url, const QString& title);
    Q_INVOKABLE void removeBookmark(const QString& url);
    Q_INVOKABLE bool contains(const QString& url) const;
    Q_INVOKABLE void save();
    Q_INVOKABLE void load();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

signals:
    void countChanged();

private:
    QMultiMap<QString, int> bookmarks;
    QList<Bookmark*> titles;
};
#endif // DECLARATIVEBOOKMARKMODEL_H
