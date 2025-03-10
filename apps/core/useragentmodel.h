/****************************************************************************
**
** Copyright (c) 2022 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef USERAGENTMODEL_H
#define USERAGENTMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QHash>

class UserAgent
{
public:
    UserAgent(const QString &host,
              const bool isKey,
              const QString &userAgent);

    QString host() const;
    bool isKey() const;
    void setIsKey(bool isKey);
    QString userAgent() const;
    void setUserAgent(const QString &userAgent);

private:
    QString m_host;
    bool m_isKey;
    QString m_userAgent;
};

class UserAgentModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QVariantMap currentHostUserAgent READ currentHostUserAgent NOTIFY currentHostUserAgentChanged FINAL)
    Q_PROPERTY(QString currentHost READ currentHost WRITE setCurrentHost NOTIFY currentHostChanged)

public:
    enum Roles {
        HostRole = Qt::UserRole,
        IsKeyRole,
        UserAgentRole,
        FavIconRole
    };

    UserAgentModel(QObject *parent = nullptr);

    Q_INVOKABLE void setUserAgentOverride(const QString &host, const QString &userAgent, const bool isKey);
    Q_INVOKABLE void unsetUserAgentOverride(const QString &host);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString currentHost() const;
    void setCurrentHost(const QString &host);

signals:
    void countChanged();
    void currentHostUserAgentChanged();
    void currentHostChanged();

private:
    QVariantMap currentHostUserAgent() const;
    int findHostIndex(const QString &host) const;

private:
    // <host, index>
    QHash<QString, int> m_index;
    QList<UserAgent> m_userAgentList;
    QString m_currentHost;
};

#endif // USERAGENTMODEL_H
