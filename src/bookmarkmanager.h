/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include <QObject>
#include <QMap>

class Bookmark;

class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    static BookmarkManager* instance();

    void save(const QMap<QString, Bookmark*> & bookmarks);
    void clear();
    QMap<QString, Bookmark*> load();

signals:
    void cleared();

private:
    BookmarkManager();

};

#endif // BOOKMARKMANAGER_H
