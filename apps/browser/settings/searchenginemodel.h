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

    Q_ENUMS(Status)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        UrlRole = Qt::UserRole + 1,
        TitleRole,
        StatusRole
    };

    enum Status {
        System,
        Available,
        UserInstalled
    };

    explicit SearchEngineModel(QObject *parent = 0);
    virtual ~SearchEngineModel();

    QStringList searchEngines();
    Q_INVOKABLE void add(const QString &title, const QString &url);
    Q_INVOKABLE void install(const QString &title);
    Q_INVOKABLE void remove(const QString &title);

    // From QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // From QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

signals:
    void countChanged();
    void installed(const QString &title);

private:
    struct SearchEngine {
        SearchEngine(const QUrl &url, const QString &title, Status status)
            : url(url)
            , title(title)
            , status(status)
        {}

        QUrl url;
        QString title;
        Status status;
    };

    QList<SearchEngine> m_searchEngines;
};

#endif
