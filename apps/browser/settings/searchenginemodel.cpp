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

SearchEngineModel::SearchEngineModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QStringList searchEngineList = OpenSearchConfigs::getSearchEngineList();
    for (const QString &name : searchEngineList) {
        SearchEngine engine(QUrl(), name, true);
        m_searchEngines.append(engine);
    }

    // Swap to real data
//    m_searchEngines.append(SearchEngine(QUrl("www.twitter.com"), "Twitter", false));
//    m_searchEngines.append(SearchEngine(QUrl("forum.sailfishos.org"), "Sailfish OS Forum", false));
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

void SearchEngineModel::addEngine(const QUrl &url, const QString &title, bool installed)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_searchEngines.append(SearchEngine(url, title, installed));
    endInsertRows();
}

void SearchEngineModel::classBegin()
{

}

void SearchEngineModel::componentComplete()
{

}
