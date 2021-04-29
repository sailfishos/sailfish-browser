/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LOGININFO_H
#define LOGININFO_H

#include <qstring.h>
#include <qvariant.h>

class LoginInfo
{
public:
    LoginInfo(const QVariantMap &data);

    QVariantMap toMap();

    QString hostname() const;
    QString username() const;
    QString password() const;

    void setUsername(const QString &username);
    void setPassword(const QString &password);

    bool doLoginsMatch(const LoginInfo &other, bool ignorePassword = false, bool ignoreSchemes = false) const;

private:
    // nsILoginInfo carefully distinguishes between null and emtpy strings
    // So we must be careful to capture the same distinction too
    static QString extractValue(QVariantMap const &data, QString const &key);
    static void insertValue(QVariantMap &data, QString const &key, QString const &value);

private:
    // The nsILoginManager users all of these fields to identify entries
    // So we need to keep track of them, even though they're not all exposed
    // to the frontend
    QString m_hostname;
    QString m_formSubmitURL;
    QString m_httpRealm;
    QString m_username;
    QString m_password;
    QString m_usernameField;
    QString m_passwordField;
};

#endif // LOGININFO_H
