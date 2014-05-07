/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
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
        TitleRole
    };

    Q_INVOKABLE void clear();
    Q_INVOKABLE void search(const QString &filter);

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    // From QQmlParserStatus
    void classBegin();
    void componentComplete();

signals:
    void countChanged();

private slots:
    void historyAvailable(QList<Link> linkList);
    void updateTitle(int tabId, int linkId, QString url, QString title);

private:
    void updateModel(QList<Link> linkList);

    QList<Link> m_links;

    friend class tst_declarativehistorymodel;
    friend class tst_webview;
};
#endif // DECLARATIVEHISTORYMODEL_H
