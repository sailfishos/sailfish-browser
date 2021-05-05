/****************************************************************************
**
** Copyright (c) 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "searchenginemodel.h"
#include "opensearchconfigs.h"
#include "datafetcher.h"

#include <QString>
#include <QFile>

#include <MGConfItem>
#include <webengine.h>

const auto SEARCH_ENGINE_CONFIG = QStringLiteral("/apps/sailfish-browser/settings/search_engine");
const auto SEARCH_ENGINES_AVAILABLE_CONFIG = QStringLiteral("/apps/sailfish-browser/settings/search_engines_available");

SearchEngineModel::SearchEngineModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QString userSearchPrefix = OpenSearchConfigs::getOpenSearchConfigPath();
    QMap<QString, QString> searchConfigs = OpenSearchConfigs::getAvailableOpenSearchConfigs();
    Status status;

    for (const QString &name : searchConfigs.keys()) {
        // User installed searches are saved to user config dir
        if (searchConfigs.value(name).startsWith(userSearchPrefix)) status = Status::UserInstalled;
        else status = Status::System;

        SearchEngine engine(QUrl(), name, status);
        m_searchEngines.append(engine);
    }

    // Get available searches from config
    MGConfItem gconf(SEARCH_ENGINES_AVAILABLE_CONFIG);
    QMap<QString, QVariant> engines = gconf.value().toMap();
    for (const QString &name : engines.keys()) {
        SearchEngine engine(engines.value(name).toString(), name, Status::Available);
        m_searchEngines.append(engine);
    }
}

SearchEngineModel::~SearchEngineModel()
{

}

int SearchEngineModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_searchEngines.count();
}

QVariant SearchEngineModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_searchEngines.count())
        return QVariant();

    const SearchEngine &searchEngine = m_searchEngines.at(index.row());
    switch (role) {
    case UrlRole:
        return searchEngine.url;
    case TitleRole:
        return searchEngine.title;
    case StatusRole:
        return searchEngine.status;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> SearchEngineModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[TitleRole] = "title";
    roles[StatusRole] = "status";
    return roles;
}

void SearchEngineModel::classBegin()
{

}

void SearchEngineModel::componentComplete()
{

}

void SearchEngineModel::add(const QString &title, const QString &url)
{
    for (const SearchEngine& engine : m_searchEngines) {
        if (engine.title == title) {
            return;
        }
    }
    SearchEngine engine(url, title, Status::Available);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_searchEngines.append(engine);
    endInsertRows();
    emit countChanged();

    MGConfItem gconf(SEARCH_ENGINES_AVAILABLE_CONFIG);
    QMap<QString, QVariant> engines = gconf.value().toMap();
    engines.insert(title, url);
    gconf.set(engines);
}

void SearchEngineModel::install(const QString &title)
{
    for (int i = 0; i < m_searchEngines.count(); i++) {
        if (m_searchEngines[i].title == title && m_searchEngines[i].status == Status::Available) {
            DataFetcher *fetcher = new DataFetcher();
            fetcher->setType(DataFetcher::Type::OpenSearch);
            connect(fetcher, &DataFetcher::statusChanged, [this, fetcher, i]() {
                if (fetcher->status() == DataFetcher::Status::Ready) {
                    m_searchEngines[i].status = Status::UserInstalled;
                    emit dataChanged(index(i), index(i), QVector<int>() << StatusRole);

                    MGConfItem gconf(SEARCH_ENGINE_CONFIG);
                    gconf.set(m_searchEngines[i].title);
                    MGConfItem available(SEARCH_ENGINES_AVAILABLE_CONFIG);
                    QMap<QString, QVariant> engines = available.value().toMap();
                    engines.remove(m_searchEngines[i].title);
                    available.set(engines);

                    emit installed(m_searchEngines[i].title);

                    fetcher->deleteLater();
                } else if (fetcher->status() == DataFetcher::Status::Error) {
                    // TODO: error notification
                    fetcher->deleteLater();
                }
            });
            fetcher->fetch(m_searchEngines[i].url.toString());
            break;
        }
    }
}

void SearchEngineModel::remove(const QString &title)
{
    MGConfItem gconf(SEARCH_ENGINE_CONFIG);
    if (gconf.value() == title)
        return;

    for (int i = 0; i < m_searchEngines.count(); i++) {
        if (m_searchEngines[i].title == title && m_searchEngines[i].status != Status::System) {
            beginRemoveRows(QModelIndex(), i, i);
            if (m_searchEngines[i].status == Status::Available) {
                MGConfItem gconf(SEARCH_ENGINES_AVAILABLE_CONFIG);
                QMap<QString, QVariant> engines = gconf.value().toMap();
                engines.remove(title);
                gconf.set(engines);
            } else {
                QFile::remove(OpenSearchConfigs::getAvailableOpenSearchConfigs().value(title));
                // Inform WebEngine
                QVariantMap message;
                message.insert(QLatin1String("msg"), QVariant(QLatin1String("remove")));
                message.insert(QLatin1String("name"), QVariant(title));
                SailfishOS::WebEngine::instance()->notifyObservers(QLatin1String("embedui:search"), QVariant(message));
            }
            m_searchEngines.removeAt(i);
            endRemoveRows();
            break;
        }
    }
}
