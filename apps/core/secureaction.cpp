/****************************************************************************
**
** Copyright (c) 2018 - 2021 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "secureaction.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusServiceWatcher>

#include <QQmlInfo>

namespace {

const QString deviceLockService = QStringLiteral("org.nemomobile.devicelock");
const QString authenticatorPath = QStringLiteral("/authenticator");
const QString authenticatorInterface = QStringLiteral("org.nemomobile.devicelock.Authenticator");

static const QString dbusService = QStringLiteral("org.freedesktop.DBus");
static const QString dbusPath = QStringLiteral("/org/freedesktop/DBus");
static const QString dbusInterface = QStringLiteral("org.freedesktop.DBus");

QDBusMessage createAuthenticatorCall(const QString &member, const QVariantList &arguments)
{
    QDBusMessage methodCall = QDBusMessage::createMethodCall(
                deviceLockService,
                authenticatorPath,
                authenticatorInterface,
                member);
    methodCall.setArguments(arguments);
    return methodCall;
}

QString replyPath()
{
    static int counter = 0;

    return QStringLiteral("/org/nemomobile/devicelock/client/sailfish_browser/SecureAction%1").arg(++counter);
}

enum class Method {
    NoAuthentication    = 0x0000,
    SecurityCode        = 0x0001,
    Fingerprint         = 0x0002,
    Confirmation        = 0x1000,
    AllAvailable = SecurityCode | Fingerprint | Confirmation
};

}

class SecureAction::ServiceWatcher : public QDBusServiceWatcher
{
    Q_OBJECT
public:
    ServiceWatcher()
        : QDBusServiceWatcher(deviceLockService, QDBusConnection::systemBus())
    {
        connect(this, &QDBusServiceWatcher::serviceOwnerChanged,
                this, [this](const QString &, const QString &oldOwner, const QString &newOwner) {
            if (newOwner.isEmpty() && m_available) {
                m_available = false;
                emit availableChanged(false);
            } else if (oldOwner.isEmpty() && !m_available) {
                m_available = true;
                emit availableChanged(true);
            }
        });

        QDBusMessage method = QDBusMessage::createMethodCall(
                    dbusService, dbusPath, dbusInterface, QStringLiteral("NameHasOwner"));
        method.setArguments({ deviceLockService });
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                    QDBusConnection::systemBus().asyncCall(method));
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();

            QDBusReply<bool> reply = *watcher;

            if (!reply.isValid()) {
                qWarning() << "Error querying" << deviceLockService << "on the system bus";
            } else if (reply.value()) {
                m_available = true;
                emit availableChanged(true);
            } else {
                qWarning() << deviceLockService << "is not available on the system bus";
            }
        });
    }

    static std::shared_ptr<ServiceWatcher> instance()
    {
        static std::weak_ptr<ServiceWatcher> instance;

        std::shared_ptr<ServiceWatcher> watcher = instance.lock();

        if (!watcher) {
            watcher = std::make_shared<ServiceWatcher>();

            instance = watcher;
        }

        return watcher;
    }

    bool isAvailable() const { return m_available; }

signals:
    void availableChanged(bool available);

private:
    bool m_available = false;
};

SecureActionAuthenticatorAdaptor::SecureActionAuthenticatorAdaptor(SecureAction *action)
    : QDBusAbstractAdaptor(action)
    , m_secureAction(action)
{
}

void SecureActionAuthenticatorAdaptor::PermissionGranted(uint)
{
    if (m_secureAction->m_authenticating) {
        QJSValue resolve = m_secureAction->m_resolve;

        m_secureAction->m_authorized = true;
        m_secureAction->m_authenticating = false;
        m_secureAction->m_resolve = QJSValue();

        resolve.call();
    }
}

void SecureActionAuthenticatorAdaptor::Aborted()
{
    m_secureAction->m_authenticating = false;
    m_secureAction->m_resolve = QJSValue();
}

SecureAction::SecureAction(QObject *parent)
    : QObject(parent)
    , m_serviceWatcher(ServiceWatcher::instance())
    , m_replyPath(replyPath())
    , m_available(m_serviceWatcher->isAvailable())
{
    connect(m_serviceWatcher.get(), &ServiceWatcher::availableChanged, this, &SecureAction::setAvailable);
}

SecureAction::~SecureAction()
{
    cancel();
}

bool SecureAction::isAvailable() const
{
    return m_available;
}

QString SecureAction::message() const
{
    return m_message;
}

void SecureAction::setMessage(const QString &message)
{
    if (m_message != message) {
        m_message = message;

        m_authorized = false;
        cancel();

        emit messageChanged();
    }
}

void SecureAction::perform(const QJSValue &resolve)
{
    if (m_authorized) {
        QJSValue(resolve).call();
    } else if (m_available && !m_message.isEmpty()) {
        m_resolve = resolve;

        if (m_authenticating) {
            return;
        }

        QDBusConnection systemBus = QDBusConnection::systemBus();

        if (!m_registered) {
            m_registered = systemBus.registerObject(m_replyPath, this);

            if (!m_registered) {
                qmlInfo(this) << "DBus adaptor registration error " << systemBus.lastError().name() << " " << systemBus.lastError().message();
                return;
            }
        }

        m_authenticating = true;

        m_response.reset(new QDBusPendingCallWatcher(systemBus.asyncCall(createAuthenticatorCall(
                    QStringLiteral("RequestPermission"),
                    { QVariant::fromValue(QDBusObjectPath(m_replyPath)), m_message, QVariantMap(), uint(Method::AllAvailable) }))));

        // The reply here just confirms that the device lock service received the request and is
        // handling it. The users response or further status changes will be delivered by
        // the device lock service invoking methods on the SecureActionAuthenticatorAdaptor
        // registered on m_replyPath.
        connect(m_response.data(), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();
            m_response.take();

            if (watcher->isError()) {
                m_authenticating = false;

                qmlInfo(this) << "Authentication D-Bus error " << watcher->error().name() << " " << watcher->error().message();
            }
        });
    }
}


void SecureAction::setAvailable(bool available)
{
    if (m_available != available) {
        m_available = available;

        if (!m_available && m_authenticating) {
            m_authenticating = false;
            m_resolve = QJSValue();
            m_response.reset();
        }

        emit availableChanged();
    }
}

void SecureAction::cancel()
{
    if (m_authenticating) {
        QDBusConnection::systemBus().send(createAuthenticatorCall(
                    QStringLiteral("Cancel"), { QVariant::fromValue(QDBusObjectPath(m_replyPath)) }));
    }

    m_resolve = QJSValue();
    m_authenticating = false;
    m_response.reset();
}

#include "secureaction.moc"
