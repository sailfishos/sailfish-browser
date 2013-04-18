/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/
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

private:
    BrowserService *m_BrowserService;
};

#endif // DBUSADAPTOR_H
