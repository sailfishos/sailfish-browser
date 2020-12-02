/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CAPTIVEPORTAL_ADAPTOR_H
#define CAPTIVEPORTAL_ADAPTOR_H

#include <QDBusAbstractAdaptor>
#include "captiveportalservice.h"

class CaptivePortalAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfishos.captiveportal")

public:
    CaptivePortalAdaptor(CaptivePortalService *browserService);

public slots:
    void openUrl(const QStringList &args);
    void closeBrowser();
private:
    CaptivePortalService *m_captivePortalService;
};

#endif
