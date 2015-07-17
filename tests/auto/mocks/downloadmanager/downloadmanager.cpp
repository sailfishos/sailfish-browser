/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "downloadmanager.h"

DownloadManager::DownloadManager()
    : QObject()
{
}

DownloadManager *DownloadManager::instance()
{
    static DownloadManager *s_singleton = 0;
    if (!s_singleton) {
        s_singleton = new DownloadManager();
    }
    return s_singleton;
}

bool DownloadManager::initialized()
{
    return true;
}
