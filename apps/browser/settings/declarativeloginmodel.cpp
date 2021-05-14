/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include <QPair>

#include "webengine.h"
#include "faviconmanager.h"

#include "declarativeloginmodel.h"

static const auto ALL_LOGINS = QStringLiteral("embed:all-logins");
static const auto LOGINS_ACTION = QStringLiteral("embedui:logins");

DeclarativeLoginModel::DeclarativeLoginModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_nextUid(0)
    , m_populated(false)
{
    SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();
    webEngine->addObserver(ALL_LOGINS);
    connect(webEngine, &SailfishOS::WebEngine::recvObserve, this, &DeclarativeLoginModel::handleRecvObserve);
}

void DeclarativeLoginModel::classBegin()
{
}

void DeclarativeLoginModel::componentComplete()
{
    requestLogins();
}

int DeclarativeLoginModel::indexFromUid(int uid) const
{
    int index = m_index.value(uid, -1);
    return ((index >= 0) && (index < m_logins.count())) ? index : -1;
}

void DeclarativeLoginModel::modify(int uid, const QString &username, const QString &password)
{
    int index = indexFromUid(uid);
    if (index < 0) {
        qWarning() << "Invalid uid when modifying login model";
        return;
    }
    LoginInfo oldLogin = m_logins.at(index).second;

    if ((oldLogin.username() == username) && (oldLogin.password() == password)) {
        return;
    }

    if (!canModify(uid, username, password)) {
        qWarning() << "Can't modify login entry to one that already exists";
        return;
    }

    LoginInfo newLogin = oldLogin;
    newLogin.setUsername(username);
    newLogin.setPassword(password);

    QVariantMap data;
    data.insert(QStringLiteral("action"), "modify");
    data.insert(QStringLiteral("oldinfo"), oldLogin.toMap());
    data.insert(QStringLiteral("newinfo"), newLogin.toMap());

    SailfishOS::WebEngine::instance()->notifyObservers(LOGINS_ACTION, QVariant(data));
    m_logins.replace(index, UidLoginInfo(uid, newLogin));

    emit dataChanged(QAbstractListModel::index(index), QAbstractListModel::index(index), QVector<int>());
}

QVariant DeclarativeLoginModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_logins.count())
        return QVariant();

    switch (role) {
    case UidRole:
        return m_logins.at(index.row()).first;
    case HostnameRole:
        return m_logins.at(index.row()).second.hostname();
    case UsernameRole:
        return m_logins.at(index.row()).second.username();
    case PasswordRole:
        return m_logins.at(index.row()).second.password();
    case FavIconRole:
        return FaviconManager::instance()->get("logins", m_logins.at(index.row()).second.hostname());
    default:
        return QVariant();
    }
}

int DeclarativeLoginModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_logins.count();
}

QHash<int, QByteArray> DeclarativeLoginModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UidRole] = "uid";
    roles[HostnameRole] = "hostname";
    roles[UsernameRole] = "username";
    roles[PasswordRole] = "password";
    roles[FavIconRole] = "favicon";
    return roles;
}

bool DeclarativeLoginModel::populated() const
{
    return m_populated;
}

void DeclarativeLoginModel::handleRecvObserve(const QString &message, const QVariant &data)
{
    if (message == ALL_LOGINS) {
        setLoginList(qvariant_cast<QVariantList>(data));
        // We're only interested in receiving the message once
        SailfishOS::WebEngine::instance()->removeObserver(ALL_LOGINS);
    }
}

void DeclarativeLoginModel::setLoginList(const QVariantList &data)
{
    beginResetModel();

    m_index.clear();
    m_logins.clear();

    for (const auto &iter : data) {
        QVariantMap varMap = iter.toMap();
        m_index.insert(m_nextUid, m_logins.count());
        m_logins.append(UidLoginInfo(m_nextUid, LoginInfo(varMap)));
        ++m_nextUid;
    }

    endResetModel();
    emit countChanged();

    if (!m_populated) {
        m_populated = true;
        emit populatedChanged();
    }
}

void DeclarativeLoginModel::requestLogins()
{
    QVariantMap data;
    data.insert(QStringLiteral("action"), "getall");
    SailfishOS::WebEngine::instance()->notifyObservers(LOGINS_ACTION, QVariant(data));
}

void DeclarativeLoginModel::remove(int uid)
{
    int index = indexFromUid(uid);
    if (index < 0) {
        qWarning() << "Invalid uid when removing item from login model";
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);

    LoginInfo login = m_logins.at(index).second;
    QVariantMap data;
    data.insert(QStringLiteral("action"), "remove");
    data.insert(QStringLiteral("login"), login.toMap());
    SailfishOS::WebEngine::instance()->notifyObservers(LOGINS_ACTION, QVariant(data));
    m_logins.removeAt(index);
    m_index.remove(uid);

    // Update the index (every item above has its position decremented)
    for (int pos = index; pos < m_logins.count(); ++pos) {
        m_index.insert(m_logins.at(pos).first, pos);
    }

    endRemoveRows();
    emit countChanged();

    // Check to see whether the list still contains this hostname
    bool containsHostname = false;
    QList<UidLoginInfo>::ConstIterator iter = m_logins.constBegin();
    while (!containsHostname && iter != m_logins.constEnd()) {
        containsHostname = ((*iter).second.hostname() == login.hostname());
        ++iter;
    }
    if (!containsHostname) {
        // This is the last of its kind, so we can remove its icon
        FaviconManager::instance()->remove("logins", login.hostname());
    }
}

bool DeclarativeLoginModel::canModify(int uid, const QString &username, const QString &password) const
{
    int index = indexFromUid(uid);
    if (index < 0) {
        qWarning() << "Invalid uid when checking login model";
        return false;
    }

    bool validModification = true;
    LoginInfo update = m_logins.at(index).second;
    update.setUsername(username);
    update.setPassword(password);

    for (int pos = 0; pos < m_logins.count() && validModification; ++pos) {
        // Intended to follow the matching criteria in LoginManagerStorage_json.modifyLogin()
        // See gecko-dev/toolkit/components/passwordmgr/storage-json.js
        if ((pos != index) && update.doLoginsMatch(m_logins.at(pos).second, true)) {
            validModification = false;
        }
    }
    return validModification;
}

