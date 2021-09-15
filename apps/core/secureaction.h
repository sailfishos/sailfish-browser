/****************************************************************************
**
** Copyright (c) 2018 - 2021 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SECUREACTION_H
#define SECUREACTION_H

#include <QObject>
#include <QDBusAbstractAdaptor>

#include <QJSValue>

#include <memory>

QT_BEGIN_NAMESPACE
class QDBusPendingCallWatcher;
QT_END_NAMESPACE

class SecureAction;

class SecureActionAuthenticatorAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.devicelock.client.Authenticator")
public:
    explicit SecureActionAuthenticatorAdaptor(SecureAction *action);

public slots:
    Q_NOREPLY void PermissionGranted(uint method);
    Q_NOREPLY void Aborted();

private:
    SecureAction * const m_secureAction;
};

class SecureAction : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged)

public:
    explicit SecureAction(QObject *parent = nullptr);
    ~SecureAction() override;

    bool isAvailable() const;

    QString message() const;
    void setMessage(const QString &message);

    Q_INVOKABLE void perform(const QJSValue &resolve);

signals:
    void availableChanged();
    void messageChanged();

private:
    friend class SecureActionAuthenticatorAdaptor;
    class ServiceWatcher;

    void setAvailable(bool available);
    void cancel();

    SecureActionAuthenticatorAdaptor m_adaptor { this };

    std::shared_ptr<ServiceWatcher> m_serviceWatcher;
    QScopedPointer<QDBusPendingCallWatcher> m_response;
    QJSValue m_resolve;
    const QString m_replyPath;
    QString m_message;
    bool m_available = false;
    bool m_authorized = false;
    bool m_authenticating = false;
    bool m_registered = false;
};

#endif
