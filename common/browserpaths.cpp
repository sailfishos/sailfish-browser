/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDebug>
#include <QString>
#include <QDir>
#include <QStandardPaths>
#include "browserpaths.h"

static QString getLocation(QStandardPaths::StandardLocation locationType) {
    QString location(QStandardPaths::writableLocation(locationType));
    QDir dir(location);
    if (!dir.exists()) {
        if (!dir.mkpath(location)) {
            qWarning() << QString("Can't create directory %1").arg(location);
            return QString();
        }
    }

    return location;
}

QString BrowserPaths::downloadLocation()
{
    return getLocation(QStandardPaths::DownloadLocation);
}

QString BrowserPaths::picturesLocation()
{
    return getLocation(QStandardPaths::PicturesLocation);
}

QString BrowserPaths::dataLocation()
{
    return getLocation(QStandardPaths::DataLocation);
}

QString BrowserPaths::applicationsLocation()
{
    return getLocation(QStandardPaths::ApplicationsLocation);
}

QString BrowserPaths::cacheLocation()
{
    return getLocation(QStandardPaths::CacheLocation);
}
