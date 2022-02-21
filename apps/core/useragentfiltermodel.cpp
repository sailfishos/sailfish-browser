/****************************************************************************
**
** Copyright (c) 2022 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "useragentfiltermodel.h"
#include "useragentmodel.h"

UserAgentFilterModel::UserAgentFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

int UserAgentFilterModel::getIndex(int currentIndex)
{
    QModelIndex proxyIndex = index(currentIndex, 0);
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    return sourceIndex.row();
}

bool UserAgentFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if (sourceModel()->data(index, UserAgentModel::HostRole).toString().trimmed().contains(m_search, Qt::CaseInsensitive)) {
        return true;
    }
    return false;
}

void UserAgentFilterModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (sourceModel) {
        beginResetModel();
        QSortFilterProxyModel::setSourceModel(sourceModel);
        endResetModel();
    }
}

QString UserAgentFilterModel::search() const
{
    return m_search;
}

void UserAgentFilterModel::setSearch(const QString &search)
{
    if (m_search == search) {
        return;
    }

    m_search = search;
    invalidateFilter();
    emit searchChanged(m_search);
}
