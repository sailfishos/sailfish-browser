/****************************************************************************
**
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC
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
    Q_PROPERTY(int maxDisplayedItems READ maxDisplayedItems WRITE setMaxDisplayedItems NOTIFY maxDisplayedItemsChanged)
public:
    BookmarkFilterModel(QObject *parent = nullptr);

    Q_INVOKABLE int getIndex(int currentIndex);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QString search() const;
    int maxDisplayedItems() const;
    void setSearch(const QString &search);
    void setMaxDisplayedItems(const int maxDisplayedItems);

signals:
    void searchChanged(QString search);
    void maxDisplayedItemsChanged(int maxDisplayedItems);

private:
    void resetCounts();
    void extendCheckPosition(int sourceRow, const QModelIndex &sourceParent) const;
    bool filterAcceptsRowUnbounded(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QString m_search;
    int m_maxDisplayedItems;
    mutable int m_countFilterAccepts;
    mutable int m_sourceMaxAccept;
    mutable int m_maxSourceModelPosition;
};

#endif // BOOKMARKFILTERMODEL_H
