/****************************************************************************
**
** Copyright (c) 2022 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef USERAGENTMANAGER_H
#define USERAGENTMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QVariant>

class UserAgentManager : public QObject
{
    Q_OBJECT

public:
    static UserAgentManager *instance();
    QVariantMap getUserAgentOverrides() const;
    void setUserAgentOverride(const QString &host, const QString &userAgent, const bool isKey);
    void unsetUserAgentOverride(const QString &host);
    Q_INVOKABLE void clearUserAgentOverrides();

public slots:
    QVariantList getBrowserList() const;

private:
    explicit UserAgentManager(QObject *parent = nullptr);
    ~UserAgentManager();

    void initialize();
    QVariantList m_browserList;
};

#endif
