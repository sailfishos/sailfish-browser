/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Mäkeläinen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linkvalidator.h"
#include <QUrl>
#include <QStringList>

const static QStringList UNNAVIGABLE_SCHEMES = QStringList() << "tel"
                                                         << "sms"
                                                         << "mailto"
                                                         << "geo";

/*!
    Returns true if url can be navigated. Relative urls are considered as navigable.
*/
bool LinkValidator::navigable(const QUrl &url)
{
    QString scheme = url.scheme();
    if (scheme.isEmpty()) {
        return true;
    }

    return !UNNAVIGABLE_SCHEMES.contains(scheme);
}
