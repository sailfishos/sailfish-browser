/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "loginfiltermodel.h"
#include "declarativeloginmodel.h"

LoginFilterModel::LoginFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

int LoginFilterModel::getIndex(int currentIndex)
{
    QModelIndex proxyIndex = index(currentIndex, 0);
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    return sourceIndex.row();
}

bool LoginFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if (sourceModel()->data(index, DeclarativeLoginModel::HostnameRole).toString().trimmed().contains(m_search, Qt::CaseInsensitive)) {
        return true;
    }
    return false;
}

void LoginFilterModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (sourceModel) {
        beginResetModel();
        QSortFilterProxyModel::setSourceModel(sourceModel);
        endResetModel();
    }
}

QString LoginFilterModel::search() const
{
    return m_search;
}

void LoginFilterModel::setSearch(const QString &search)
{
    if (m_search == search) {
        return;
    }

    m_search = search;
    invalidateFilter();
    emit searchChanged(m_search);
}
