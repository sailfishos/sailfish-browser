/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "declarativehistorymodel.h"

#include "dbmanager.h"

DeclarativeHistoryModel::DeclarativeHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(DBManager::instance(), SIGNAL(historyAvailable(QList<Link>)),
            this, SLOT(historyAvailable(QList<Link>)));
    connect(DBManager::instance(), SIGNAL(titleChanged(QString,QString)),
            this, SLOT(updateTitle(QString,QString)));
}

QHash<int, QByteArray> DeclarativeHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UrlRole] = "url";
    roles[TitleRole] = "title";
    return roles;
}

void DeclarativeHistoryModel::clear()
{
    beginResetModel();
    m_links.clear();
    endResetModel();
    DBManager::instance()->clearHistory();
    emit countChanged();
}

void DeclarativeHistoryModel::remove(int index)
{
    if (index < 0 || index >= m_links.count()) {
        qWarning() << "Trying to remove invalid history entry.";
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    Link link = m_links.takeAt(index);
    DBManager::instance()->removeHistoryEntry(link.linkId());
    endRemoveRows();
    emit countChanged();
}

void DeclarativeHistoryModel::search(const QString &filter)
{
    DBManager::instance()->getHistory(filter);
}

int DeclarativeHistoryModel::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return m_links.count();
}

QVariant DeclarativeHistoryModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_links.count())
        return QVariant();

    const Link url = m_links[index.row()];
    if (role == UrlRole) {
        return url.url();
    } else if (role == TitleRole) {
        return url.title();
    }
    return QVariant();
}

void DeclarativeHistoryModel::componentComplete()
{
    search("");
}

void DeclarativeHistoryModel::classBegin()
{
}

void DeclarativeHistoryModel::historyAvailable(QList<Link> linkList)
{
    // DBWorker suppresses history (distinct select). Thus, id and thumbnailPath of
    // every link is the same.
    updateModel(linkList);
}

void DeclarativeHistoryModel::updateModel(QList<Link> linkList)
{
    int i = 0;
    int startIndex = -1;
    for (i = 0; i < linkList.count() && i < m_links.count(); i++) {
        if (m_links.at(i) != linkList.at(i)) {
            m_links[i] = linkList.at(i);
            if (startIndex < 0) {
                startIndex = i;
            }
        } else if (startIndex >= 0) {
            emit dataChanged(index(startIndex), index(i-1));
            startIndex = -1;
        }
    }

    if (startIndex >= 0) {
        emit dataChanged(index(startIndex), index(m_links.count()-1));
    }

    int difference = linkList.count() - m_links.count();
    if (difference != 0) {
        if (difference < 0) {
            beginRemoveRows(QModelIndex(), linkList.count(), m_links.count()-1);
            m_links.erase(m_links.begin()+linkList.count(), m_links.end());
            endRemoveRows();
        } else {
            beginInsertRows(QModelIndex(), m_links.count(), linkList.count()-1);
            m_links.append(linkList.mid(m_links.count()));
            endInsertRows();
        }

        emit countChanged();
    }
}

void DeclarativeHistoryModel::updateTitle(QString url, QString title)
{
    QVector<int> roles;
    roles << TitleRole;
    for (int i = 0; i < m_links.count(); i++) {
        if (m_links.at(i).url() == url && m_links.at(i).title() != title) {
            m_links[i].setTitle(title);
            QModelIndex start = index(i, 0);
            QModelIndex end = index(i, 0);
            emit dataChanged(start, end, roles);
        }
    }
}
