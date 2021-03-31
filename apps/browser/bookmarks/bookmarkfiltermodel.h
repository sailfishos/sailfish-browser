/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BOOKMARKFILTERMODEL_H
#define BOOKMARKFILTERMODEL_H

#include <QSortFilterProxyModel>

class BookmarkFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
public:
    BookmarkFilterModel(QObject *parent = nullptr);

    Q_INVOKABLE int getIndex(int currentIndex);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    QString search() const;
    void setSearch(const QString &search);

signals:
    void searchChanged(QString search);

private:
    QString m_search;
};

#endif // BOOKMARKFILTERMODEL_H
