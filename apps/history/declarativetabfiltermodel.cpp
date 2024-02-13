/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativetabfiltermodel.h"
#include "declarativetabmodel.h"

DeclarativeTabFilterModel::DeclarativeTabFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

int DeclarativeTabFilterModel::getIndex(int currentIndex)
{
    QModelIndex proxyIndex = index(currentIndex, 0);
    QModelIndex sourceIndex = mapToSource(proxyIndex);
    return sourceIndex.row();
}

bool DeclarativeTabFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    return (m_showHidden || !sourceModel()->data(index, DeclarativeTabModel::HiddenRole).toBool());
}

void DeclarativeTabFilterModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (sourceModel) {
    	DeclarativeTabModel *oldModel = static_cast<DeclarativeTabModel*>(this->sourceModel());
    	DeclarativeTabModel *newModel = static_cast<DeclarativeTabModel*>(sourceModel);
    	if (oldModel) {
			disconnect(oldModel, &DeclarativeTabModel::activeTabIndexChanged, this, &DeclarativeTabFilterModel::activeTabIndexChanged);
			disconnect(oldModel, &DeclarativeTabModel::countChanged, this, &DeclarativeTabFilterModel::countChanged);
    	}
        beginResetModel();
        QSortFilterProxyModel::setSourceModel(sourceModel);
        endResetModel();
    	connect(newModel, &DeclarativeTabModel::activeTabIndexChanged, this, &DeclarativeTabFilterModel::activeTabIndexChanged);
    	connect(newModel, &DeclarativeTabModel::countChanged, this, &DeclarativeTabFilterModel::countChanged);
    }
}

bool DeclarativeTabFilterModel::showHidden() const
{
    return m_showHidden;
}

void DeclarativeTabFilterModel::setShowHidden(const bool showHidden)
{
    if (m_showHidden == showHidden) {
        return;
    }

    m_showHidden = showHidden;
    invalidateFilter();
    emit showHiddenChanged(m_showHidden);
}

int DeclarativeTabFilterModel::activeTabIndex() const
{
	int sourceTabIndex = static_cast<DeclarativeTabModel*>(sourceModel())->activeTabIndex();
	return sourceTabIndex;
	int proxyTabIndex = -1;
	if (sourceTabIndex >= 0) {
		QModelIndex sourceIndex = index(static_cast<DeclarativeTabModel*>(sourceModel())->activeTabIndex(), 0);
		QModelIndex proxyIndex = mapFromSource(sourceIndex);
		proxyTabIndex = proxyIndex.row();
	}
	qDebug() << "PRINT: activeTabIndex proxy: " << proxyTabIndex;
    return proxyTabIndex;
}

int DeclarativeTabFilterModel::count() const
{
	return rowCount();
}

