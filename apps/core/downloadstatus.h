/****************************************************************************
**
** Copyright (c) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOWNLOAD_STATUS_H
#define DOWNLOAD_STATUS_H

#include <QObject>

class DownloadStatus : public QObject
{
    Q_OBJECT
    Q_ENUMS(Status)
public:
    enum Status {
        Started,
        Done,
        Failed,
        Canceled
    };
};

#endif
