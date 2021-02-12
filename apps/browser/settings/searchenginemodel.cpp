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
#include "../bookmarks/datafetcher.h"

#include <QString>
#include <MGConfItem>

const auto SEARCH_ENGINE_CONFIG = QStringLiteral("/apps/sailfish-browser/settings/search_engine");

SearchEngineModel::SearchEngineModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QStringList searchEngineList = OpenSearchConfigs::getSearchEngineList();
    for (const QString &name : searchEngineList) {
        SearchEngine engine(QUrl(), name, true);
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
    case InstalledRole:
        return searchEngine.installed;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> SearchEngineModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[TitleRole] = "title";
    roles[InstalledRole] = "installed";
    return roles;
}

void SearchEngineModel::classBegin()
{

}

void SearchEngineModel::componentComplete()
{

}

void SearchEngineModel::add(const QString& title, const QString& url)
{
    for (const SearchEngine& engine : m_searchEngines) {
        if (engine.title == title) {
            return;
        }
    }
    SearchEngine engine(url, title, false);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_searchEngines.append(engine);
    endInsertRows();
    emit countChanged();
}

void SearchEngineModel::install(const QString& title)
{
    for (int i = 0; i < m_searchEngines.count(); i++) {
        if (m_searchEngines[i].title == title && !m_searchEngines[i].installed) {
            DataFetcher *fetcher = new DataFetcher();
            fetcher->setType(DataFetcher::Type::OpenSearch);
            connect(fetcher, &DataFetcher::statusChanged, [this, fetcher, i]() {
                if (fetcher->status() == DataFetcher::Status::Ready) {
                    m_searchEngines[i].installed = true;
                    emit dataChanged(index(i), index(i), QVector<int>(InstalledRole));

                    MGConfItem gconf(SEARCH_ENGINE_CONFIG);
                    gconf.set(m_searchEngines[i].title);

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
