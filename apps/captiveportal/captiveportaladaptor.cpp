/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "captiveportaladaptor.h"

CaptivePortalAdaptor::CaptivePortalAdaptor(CaptivePortalService *captivePortalService)
    : QDBusAbstractAdaptor(captivePortalService)
    , m_captivePortalService(captivePortalService)
{
}

void CaptivePortalAdaptor::openUrl(const QStringList &args)
{
    m_captivePortalService->openUrl(args);
}

void CaptivePortalAdaptor::closeBrowser()
{
    m_captivePortalService->closeBrowser();
}
