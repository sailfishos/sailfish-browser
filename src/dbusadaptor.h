/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DBUSADAPTOR_H
#define DBUSADAPTOR_H

#include <QDBusAbstractAdaptor>
#include "browserservice.h"

class DBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfishos.browser")

public:
    DBusAdaptor(BrowserService *browserService);

public slots:
    void openUrl(QStringList args);
    void cancelTransfer(int transferId);
    void restartTransfer(int transferId);
    void dumpMemoryInfo(QString fileName);

private:
    BrowserService *m_BrowserService;
};

#endif // DBUSADAPTOR_H
