/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BROWSERSERVICE_H
#define BROWSERSERVICE_H

#include <QObject>
#include <QStringList>

class BrowserService : public QObject
{
    Q_OBJECT
public:
    BrowserService(QObject * parent);
    bool registered() const;
    QString serviceName() const;

public slots:
    void openUrl(QStringList args);
    void cancelTransfer(int transferId);
    void restartTransfer(int transferId);
    void dumpMemoryInfo(QString fileName);

signals:
    void openUrlRequested(QString url);
    void cancelTransferRequested(int transferId);
    void restartTransferRequested(int transferId);
    void dumpMemoryInfoRequested(QString fileName);

private:
    bool m_registered;
};

#endif // BROWSERSERVICE_H
