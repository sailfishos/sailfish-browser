/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DECLARATIVEHISTORYMODEL_H
#define DECLARATIVEHISTORYMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

#include "tab.h"
#include "link.h"

class DeclarativeHistoryModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    DeclarativeHistoryModel(QObject *parent = 0);

    enum UrlRoles {
        UrlRole = Qt::UserRole + 1,
        TitleRole,
        DateRole,
        FaviconRole
    };

    Q_INVOKABLE void clear();
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void remove(const QString &url);
    Q_INVOKABLE void search(const QString &filter);
    Q_INVOKABLE void add(const QString &url, const QString &title);
    Q_INVOKABLE void setTitle(const QString &url, const QString &title);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // From QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

signals:
    void countChanged();
    void populated();

private slots:
    void historyAvailable(QList<Link> linkList);
    void updateTitle(const QString &url, const QString &title);

private:
    void updateModel(QList<Link> linkList);

    QList<Link> m_links;
    QString m_searchTerm;
    bool m_populated;

    friend class tst_declarativehistorymodel;
    friend class tst_webview;
};
#endif // DECLARATIVEHISTORYMODEL_H
