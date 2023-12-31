/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVETABFILTERMODEL_H
#define DECLARATIVETABFILTERMODEL_H

#include <QSortFilterProxyModel>

class DeclarativeTabFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(int activeTabIndex READ activeTabIndex NOTIFY activeTabIndexChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    DeclarativeTabFilterModel(QObject *parent = nullptr);

    Q_INVOKABLE int getIndex(int currentIndex);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    bool showHidden() const;
    void setShowHidden(const bool showHidden);

    int activeTabIndex() const;
    int count() const;

signals:
    void showHiddenChanged(bool showHidden);
    void activeTabIndexChanged();
    void countChanged();

private:
    bool m_showHidden;
};

#endif // DECLARATIVETABFILTERMODEL_H
