/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <qurl.h>

#include "logininfo.h"

LoginInfo::LoginInfo(const QVariantMap &data)
    : m_hostname(extractValue(data, QStringLiteral("hostname")))
    , m_formSubmitURL(extractValue(data, QStringLiteral("formSubmitURL")))
    , m_httpRealm(extractValue(data, QStringLiteral("httpRealm")))
    , m_username(extractValue(data, QStringLiteral("username")))
    , m_password(extractValue(data, QStringLiteral("password")))
    , m_usernameField(extractValue(data, QStringLiteral("usernameField")))
    , m_passwordField(extractValue(data, QStringLiteral("passwordField")))
{
}

// nsILoginInfo carefully distinguishes between null and emtpy strings
// So we must be careful to capture the same distinction too
QString LoginInfo::extractValue(QVariantMap const &data, QString const &key) {
    QString result;
    if (data.contains(key) && data.value(key).isValid()) {
        result = data.value(key).toString();
    }
    return result;
}

void LoginInfo::insertValue(QVariantMap &data, QString const &key, QString const &value) {
    !value.isNull() ? data.insert(key, value) : data.insert(key, QVariant());
}

QVariantMap LoginInfo::toMap() {
    QVariantMap data;
    insertValue(data, QStringLiteral("hostname"), m_hostname);
    insertValue(data, QStringLiteral("formSubmitURL"), m_formSubmitURL);
    insertValue(data, QStringLiteral("httpRealm"), m_httpRealm);
    insertValue(data, QStringLiteral("username"), m_username);
    insertValue(data, QStringLiteral("password"), m_password);
    insertValue(data, QStringLiteral("usernameField"), m_usernameField);
    insertValue(data, QStringLiteral("passwordField"), m_passwordField);
    return data;
}

QString LoginInfo::hostname() const
{
    return m_hostname;
}

QString LoginInfo::username() const
{
    return m_username;
}

QString LoginInfo::password() const
{
    return m_password;
}

void LoginInfo::setUsername(const QString &username)
{
    m_username = username;
}

void LoginInfo::setPassword(const QString &password)
{
    m_password = password;
}

bool LoginInfo::doLoginsMatch(const LoginInfo &other, bool ignorePassword, bool ignoreSchemes) const
{
    // This is designed to match the behaviour of LoginHelper.doLoginsMatch
    // See gecko-dev/toolkit/components/passwordmgr/LoginHelper.jsm

    if (m_httpRealm != other.m_httpRealm || m_username != other.m_username) {
        return false;
    }

    if (!ignorePassword && m_password != other.m_password) {
        return false;
    }

    if (ignoreSchemes) {
        QUrl hostname1URI(m_hostname);
        QUrl hostname2URI(other.m_hostname);
        if (hostname1URI.port() != hostname2URI.port()) {
            return false;
        }

        QUrl formHostname1URI(m_formSubmitURL);
        QUrl formHostname2URI(other.m_formSubmitURL);
        if (m_formSubmitURL != "" && other.m_formSubmitURL != ""
                && formHostname1URI.port() != formHostname2URI.port()) {
            return false;
        }
    } else {
        if (m_hostname != other.m_hostname) {
            return false;
        }

        // If either formSubmitURL is blank (but not null), then match.
        if (m_formSubmitURL != "" && other.m_formSubmitURL != ""
                && ((m_formSubmitURL.isNull() != other.m_formSubmitURL.isNull())
                    || m_formSubmitURL != other.m_formSubmitURL)) {
            return false;
        }
    }

    // The .usernameField and .passwordField values are ignored.

    return true;
}
