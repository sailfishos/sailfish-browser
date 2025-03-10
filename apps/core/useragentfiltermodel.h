/****************************************************************************
**
** Copyright (c) 2022 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef USERAGENTFILTERMODEL_H
#define USERAGENTFILTERMODEL_H

#include <QSortFilterProxyModel>

class UserAgentFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
public:
    UserAgentFilterModel(QObject *parent = nullptr);

    Q_INVOKABLE int getIndex(int currentIndex);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QString search() const;
    void setSearch(const QString &search);

signals:
    void searchChanged(QString search);

private:
    QString m_search;
};

#endif // USERAGENTFILTERMODEL_H
