/****************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOWNLOAD_MIMETYPE_HANDLER_H
#define DOWNLOAD_MIMETYPE_HANDLER_H

#include <QString>

class DownloadMimetypeHandler
{
public:
    static void update();

private:
    static void appendDefaults(const QString &targetFile);
    static int hasDefaults(const QString &targetFile);

    friend class tst_downloadmimetypehandler;
};

#endif
