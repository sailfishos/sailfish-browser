/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LOGINFILTERMODEL_H
#define LOGINFILTERMODEL_H

#include <QSortFilterProxyModel>

class LoginFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
public:
    LoginFilterModel(QObject *parent = nullptr);

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

#endif // LOGINFILTERMODEL_H
