/****************************************************************************
**
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC
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
    extendCheckPosition(sourceRow, sourceParent);
    return ((sourceRow < m_maxSourceModelPosition) && filterAcceptsRowUnbounded(sourceRow, sourceParent));
}

bool BookmarkFilterModel::filterAcceptsRowUnbounded(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if ((sourceModel()->data(index, DeclarativeBookmarkModel::UrlRole).toString().trimmed().contains(m_search, Qt::CaseInsensitive)
         || sourceModel()->data(index, DeclarativeBookmarkModel::TitleRole).toString().trimmed().contains(m_search, Qt::CaseInsensitive))) {
        return true;
    }
    return false;
}

void BookmarkFilterModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (sourceModel) {
        beginResetModel();
        resetCounts();
        m_maxDisplayedItems = sourceModel->rowCount();
        QSortFilterProxyModel::setSourceModel(sourceModel);
        endResetModel();
    }
}

QString BookmarkFilterModel::search() const
{
    return m_search;
}

int BookmarkFilterModel::maxDisplayedItems() const
{
    return m_maxDisplayedItems;
}

void BookmarkFilterModel::setSearch(const QString &search)
{
    if (m_search == search)
        return;

    m_search = search;
    resetCounts();
    invalidateFilter();
    emit searchChanged(m_search);
}

void BookmarkFilterModel::setMaxDisplayedItems(const int maxDisplayedItems)
{
    if (m_maxDisplayedItems == maxDisplayedItems)
        return;

    m_maxDisplayedItems = maxDisplayedItems;
    resetCounts();
    invalidateFilter();
    emit maxDisplayedItemsChanged(m_maxDisplayedItems);
}

void BookmarkFilterModel::resetCounts()
{
    m_maxSourceModelPosition = 0;
    m_countFilterAccepts = 0;
    m_sourceMaxAccept = 0;
}

void BookmarkFilterModel::extendCheckPosition(int sourceRow, const QModelIndex &sourceParent) const
{
    // Pushes m_maxSourceModelPosition no further than the bound in the source model
    // at which m_maxDisplayedItems will be shown in the filtered model
    while ((m_maxSourceModelPosition <= sourceRow) && (m_countFilterAccepts < m_maxDisplayedItems)) {
        if (filterAcceptsRowUnbounded(m_maxSourceModelPosition, sourceParent)) {
            ++m_countFilterAccepts;
        }
        ++m_maxSourceModelPosition;
    }
}

