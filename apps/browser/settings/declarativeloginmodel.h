/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LOGINMODEL_H
#define LOGINMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <QMap>

#include "logininfo.h"

typedef QPair<int, LoginInfo> UidLoginInfo;

class DeclarativeLoginModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool populated READ populated NOTIFY populatedChanged)
public:
    enum Roles {
        UidRole = Qt::UserRole,
        HostnameRole,
        UsernameRole,
        PasswordRole
    };

    DeclarativeLoginModel(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    Q_INVOKABLE void modify(int uid, const QString &username, const QString &password);
    Q_INVOKABLE void remove(int uid);
    Q_INVOKABLE bool canModify(int uid, const QString &username, const QString &password) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool populated() const;

signals:
    void countChanged();
    void populatedChanged();

private:
    void setLoginList(const QVariantList &data);
    void requestLogins();
    // Returns -1 if the uid is invalid
    int indexFromUid(int uid) const;

private slots:
    void handleRecvObserve(const QString &message, const QVariant &data);

private:
    // <uid, index>
    QMap<int, int> m_index;
    // <uid, data>
    QList<UidLoginInfo> m_logins;
    int m_nextUid;
    bool m_populated;
};

#endif // LOGINMODEL_H
