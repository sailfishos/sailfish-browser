/****************************************************************************
**
** Copyright (c) 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SEARCHENGINEMODEL_H
#define SEARCHENGINEMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <QList>
#include <QUrl>

class SearchEngineModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        UrlRole = Qt::UserRole + 1,
        TitleRole,
        InstalledRole
    };

    explicit SearchEngineModel(QObject *parent = 0);
    virtual ~SearchEngineModel();

    // From QAbstractListModel
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addEngine(const QUrl &url, const QString &title, bool installed);

    // From QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

signals:
    void countChanged();

private:
    struct SearchEngine {
        SearchEngine(const QUrl &url, const QString &title, bool installed)
            : url(url)
            , title(title)
            , installed(installed)
        {}

        QUrl url;
        QString title;
        bool installed;
    };

    QList<SearchEngine> m_searchEngines;
};

#endif
