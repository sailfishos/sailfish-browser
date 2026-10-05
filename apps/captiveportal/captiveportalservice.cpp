/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "captiveportalservice.h"
#include "captiveportaladaptor.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusConnectionInterface>
#include <QDBusReply>

static const auto CaptivePortalServiceName = QStringLiteral("org.sailfishos.captiveportal");

CaptivePortalService::CaptivePortalService(QObject *parent)
    : QObject(parent)
    , QDBusContext()
    , m_registered(true)
{
    new CaptivePortalAdaptor(this);
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerObject("/", this)
            || !connection.registerService(serviceName())) {
        m_registered = false;
    }
}

bool CaptivePortalService::registered() const
{
    return m_registered;
}

QString CaptivePortalService::serviceName() const
{
    return CaptivePortalServiceName;
}

void CaptivePortalService::closeBrowser()
{
    emit closeBrowserRequested();
    const QDBusMessage &msg = message();
    QDBusMessage reply = msg.createReply();
    connection().send(reply);
}

void CaptivePortalService::openUrl(const QStringList &args)
{
    if (args.count() > 0) {
        emit openUrlRequested(args.first());
    } else {
        emit openUrlRequested(QString());
    }
}


bool CaptivePortalService::transferCaller() const
{
    const QDBusReply<uint> caller = connection().interface()->servicePid(message().service());
    const QDBusReply<uint> owner = connection().interface()->servicePid(QStringLiteral("org.nemo.transferengine"));
    if (caller.isValid() && owner.isValid() && caller.value() != 0 && caller.value() == owner.value()) return true;
    sendErrorReply(QDBusError::AccessDenied, QStringLiteral("Only Transfer Engine may control downloads"));
    return false;
}

void CaptivePortalService::cancelTransfer(int transferId)
{
    if (transferCaller()) emit cancelTransferRequested(transferId);
}

void CaptivePortalService::restartTransfer(int transferId)
{
    if (transferCaller()) emit restartTransferRequested(transferId);
}
