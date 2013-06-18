/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include "declarativebookmarkmodel.h"


#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <qjson/qobjecthelper.h>
#include <qjson/parser.h>
#include <qjson/serializer.h>

#else
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#endif

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

    titles.append(new Bookmark(title, url, favicon));
    bookmarks.insert(url, titles.count()-1);
    endInsertRows();

    emit countChanged();
    save();
}

void DeclarativeBookmarkModel::removeBookmark(const QString& url) {
    if(!contains(url)) {
        return;
    }

    int index = bookmarks.take(url);
    delete titles[index];
    titles.removeAt(index);
    bookmarks.remove(url, index);

    emit countChanged();
    save();
}

void DeclarativeBookmarkModel::componentComplete() {
    QString settingsLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/bookmarks.json";
    QFile file(settingsLocation);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open bookmarks "+settingsLocation;
    } else {

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
        QJson::Parser parser;
        QVariant list = parser.parse(file.readAll());
        QVariantList bookmarkList = list.toList();
        for(int i=0; i< bookmarkList.count(); i++) {
            Bookmark* m = new Bookmark("","","");
            QJson::QObjectHelper::qvariant2qobject(bookmarkList[i].toMap(), m);
            titles.append(m);
            bookmarks.insert(m->url(), titles.count()-1);
        }


#else
        // TODO check on Qt5
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());


        if( doc.isArray()) {
            QJsonArray array = doc.array();
            QJsonArray::iterator i;
            for(i=array.begin(); i != array.end(); ++i ) {
                if((*i).isObject()) {
                    QJsonObject obj = (*i).toObject();
                    Bookmark* m = new Bookmark(obj.value("title").toString(),
                                               obj.value("url").toString(),
                                               obj.value("favicon").toString());
                    titles.append(m);
                    bookmarks.insert(m->url(), titles.count()-1);

                }
            }
        } else {
            qWarning() << "Bookmarks.json should be an array of items";
        }
#endif

        emit countChanged();
        file.close();
    }
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
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QVariantList items;
    for(int i=0; i< titles.count(); i++) {
        items << QJson::QObjectHelper::qobject2qvariant(titles[i]);
    }

    QJson::Serializer serializer;
    out << serializer.serialize(items);
#else


    QJsonArray items;
    for(int i=0; i< titles.count(); i++) {

        QJsonObject title;
        Bookmark* bookmark = titles[i];
        title.insert("url", QJsonValue(bookmark->url()));
        title.insert("title", QJsonValue(bookmark->title()));
        title.insert("favicon", QJsonValue(bookmark->favicon()));


        items.append(QJsonValue(title));
    }

    QJsonDocument doc(items);
    out << doc.toJson();
#endif

    file.close();
}

int DeclarativeBookmarkModel::rowCount(const QModelIndex & parent) const {
    return titles.count();
}

QVariant DeclarativeBookmarkModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() > titles.count())
        return QVariant();

    const Bookmark * bookmark = titles[index.row()];
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
