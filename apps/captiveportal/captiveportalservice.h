/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CAPTIVEPORTAL_SERVICE_H
#define CAPTIVEPORTAL_SERVICE_H

#include <QObject>
#include <QDBusContext>

class CaptivePortalService : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    CaptivePortalService(QObject *parent = nullptr);
    bool registered() const;
    QString serviceName() const;

public slots:
    void openUrl(const QStringList &args);
    void closeBrowser();
    void cancelTransfer(int transferId);
    void restartTransfer(int transferId);

signals:
    void openUrlRequested(const QString &url);
    void closeBrowserRequested();
    void cancelTransferRequested(int transferId);
    void restartTransferRequested(int transferId);

private:
    bool transferCaller() const;
    bool m_registered;
};

#endif
