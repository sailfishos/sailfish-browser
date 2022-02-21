/****************************************************************************
**
** Copyright (c) 2022 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "useragentmodel.h"
#include "useragentmanager.h"
#include "faviconmanager.h"

UserAgent::UserAgent(const QString &host, const bool isKey, const QString &userAgent)
    : m_host(host)
    , m_isKey(isKey)
    , m_userAgent(userAgent)
{
}

QString UserAgent::host() const
{
    return m_host;
}

bool UserAgent::isKey() const
{
    return m_isKey;
}

void UserAgent::setIsKey(bool isKey)
{
    m_isKey = isKey;
}

QString UserAgent::userAgent() const
{
    return m_userAgent;
}

void UserAgent::setUserAgent(const QString &userAgent)
{
    m_userAgent = userAgent;
}

UserAgentModel::UserAgentModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QVariantMap overrides = UserAgentManager::instance()->getUserAgentOverrides();

    beginResetModel();
        for (QVariantMap::const_iterator it = overrides.begin(); it != overrides.end(); ++it) {
        m_userAgentList.append(UserAgent(it.key(), it.value().toList().at(0).toBool(), it.value().toList().at(1).toString()));
        m_index.insert(it.key(), m_userAgentList.size() - 1);
    }
    endResetModel();
}

int UserAgentModel::findHostIndex(const QString &host) const
{
    QHash<QString, int>::const_iterator it = m_index.find(host);
    return (it != m_index.end()) ? it.value() : -1;
}

QVariantMap UserAgentModel::currentHostUserAgent() const
{
    int id = findHostIndex(m_currentHost);
    return (id >= 0) ? QVariantMap{{"isKey", m_userAgentList[id].isKey()},
                                   {"userAgent", m_userAgentList[id].userAgent()}} :
                       QVariantMap{{"isKey", true},
                                   {"userAgent", ""}};
}

void UserAgentModel::setUserAgentOverride(const QString &host, const QString &userAgent, const bool isKey)
{
    UserAgentManager::instance()->setUserAgentOverride(host, userAgent, isKey);

    int id = findHostIndex(host);
    if (id >= 0) {
        UserAgent &ua = m_userAgentList[id];

        QVector<int> roles;
        if (ua.isKey() != isKey) {
            roles << IsKeyRole;
            ua.setIsKey(isKey);
        }
        if (ua.userAgent() != userAgent) {
            roles << UserAgentRole;
            ua.setUserAgent(userAgent);
        }
        if (roles.count() > 0) {
            emit dataChanged(index(id), index(id), roles);
            if (host == m_currentHost) {
                emit currentHostUserAgentChanged();
            }
        }
    } else {
        int count = m_userAgentList.count();
        beginInsertRows(QModelIndex(), count, count);
        m_userAgentList.append(UserAgent{host, isKey, userAgent});
        m_index.insert(host, m_userAgentList.size() - 1);
        endInsertRows();

        emit countChanged();

        if (host == m_currentHost) {
            emit currentHostUserAgentChanged();
        }
    }
}

void UserAgentModel::unsetUserAgentOverride(const QString &host)
{
    UserAgentManager::instance()->unsetUserAgentOverride(host);

    int id = findHostIndex(host);
    if (id >= 0) {
        beginRemoveRows(QModelIndex(), id, id);
        m_userAgentList.removeAt(id);
        endRemoveRows();

        m_index.clear();
        for (int i = 0; i < m_userAgentList.size(); ++i) {
            m_index.insert(m_userAgentList[i].host(), i);
        }

        emit countChanged();

        if (host == m_currentHost) {
            emit currentHostUserAgentChanged();
        }
    }
}

QVariant UserAgentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_userAgentList.count())
        return QVariant();

    const UserAgent &ua = m_userAgentList.at(index.row());

    switch (role) {
    case HostRole:
        return ua.host();
    case IsKeyRole:
        return ua.isKey();
    case UserAgentRole:
        return ua.userAgent();
    case FavIconRole: {
        QString favIcon = FaviconManager::instance()->get("userAgents", "https://" + ua.host());
        if (favIcon.isEmpty()) favIcon = FaviconManager::instance()->get("userAgents", "http://" + ua.host());
        return favIcon;
    }
    default:
        return QVariant();
    }
}

int UserAgentModel::rowCount(const QModelIndex&) const
{
    return m_userAgentList.count();
}

QHash<int, QByteArray> UserAgentModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[HostRole] = "host";
    roles[IsKeyRole] = "isKey";
    roles[UserAgentRole] = "userAgent";
    roles[FavIconRole] = "favicon";
    return roles;
}

QString UserAgentModel::currentHost() const
{
    return m_currentHost;
}

void UserAgentModel::setCurrentHost(const QString &host)
{
    if (m_currentHost == host) {
        return;
    }

    m_currentHost = host;
    emit currentHostChanged();
    emit currentHostUserAgentChanged();
}
