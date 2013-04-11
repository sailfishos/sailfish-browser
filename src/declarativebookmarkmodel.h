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
#include <QDeclarativeParserStatus>

#include "bookmark.h"

class DeclarativeBookmarkModel : public QAbstractListModel, public QDeclarativeParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QDeclarativeParserStatus)

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

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    // From QDeclarativeParserStatus
    void classBegin();
    void componentComplete();

signals:
    void countChanged();

private:
    void save();

    QMultiMap<QString, int> bookmarks;
    QList<Bookmark*> titles;
};
#endif // DECLARATIVEBOOKMARKMODEL_H
