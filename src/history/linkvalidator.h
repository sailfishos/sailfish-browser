/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Mäkeläinen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINK_VALIDATOR_H
#define LINK_VALIDATOR_H

class QUrl;

class LinkValidator
{
public:
    static bool navigable(const QUrl &url);

private:
    explicit LinkValidator();
};
#endif // LINK_VALIDATOR_H
