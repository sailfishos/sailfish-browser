/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

#include "declarativebookmarkmodel.h"
#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QTextStream>

DeclarativeBookmarkModel::DeclarativeBookmarkModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[TitleRole] = "title";
    roles[FaviconRole] = "favicon";
    setRoleNames(roles);  
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
    QString settingsLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/bookmarks.json";
    QFile file(settingsLocation);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open bookmarks "+settingsLocation;
    } else {

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
        QJson::Parser parser;
        QVariant list = parser.parse(file.readAll());
        QVariantList bookmarkList = list.toList();
#else
        // TODO check on Qt5
        QJsonDocument doc = fromJson(file.readAll());
        QVariantList bookmarkList = doc.toVariantList();
#endif
        for(int i=0; i< bookmarkList.count(); i++) {
            Bookmark* m = new Bookmark("","","");
            QJson::QObjectHelper::qvariant2qobject(bookmarkList[i].toMap(), m);
            titles.append(m);
            bookmarks.insert(m->url(), titles.count()-1);
        }

        emit countChanged();
        file.close();
    }
}

void DeclarativeBookmarkModel::classBegin() {}

void DeclarativeBookmarkModel::save() {
    QVariantList items;
    for(int i=0; i< titles.count(); i++) {
        items << QJson::QObjectHelper::qobject2qvariant(titles[i]);
    }

    QString settingsLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
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
    QJson::Serializer serializer;
    out << serializer.serialize(items);
#else
    // TODO check on Qt5
    QJsonArray array = QJsonArray::fromVariantList(items);
    QJsonDocument doc(array);
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
