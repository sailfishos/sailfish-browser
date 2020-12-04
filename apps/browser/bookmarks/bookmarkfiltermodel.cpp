/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bookmarkfiltermodel.h"
#include "declarativebookmarkmodel.h"

BookmarkFilterModel::BookmarkFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

int BookmarkFilterModel::getIndex(int currentIndex)
{
    QModelIndex proxyIndex = index(currentIndex, 0);
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    return sourceIndex.row();
}

bool BookmarkFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if (sourceModel()->data(index, DeclarativeBookmarkModel::UrlRole).toString().trimmed().contains(search(), Qt::CaseInsensitive)
            || sourceModel()->data(index, DeclarativeBookmarkModel::TitleRole).toString().trimmed().contains(search(), Qt::CaseInsensitive)) {
        return true;
    }
    return false;
}

QString BookmarkFilterModel::search() const
{
    return m_search;
}

void BookmarkFilterModel::setSearch(const QString &search)
{
    if (m_search == search)
        return;

    m_search = search;
    emit searchChanged(m_search);
    invalidate();
}
