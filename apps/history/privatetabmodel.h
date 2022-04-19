/****************************************************************************
**
** Copyright (c) 2015 Jolla Ltd.
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PRIVATETABMODEL_H
#define PRIVATETABMODEL_H

#include "declarativetabmodel.h"

class DeclarativeWebContainer;

class PrivateTabModel : public DeclarativeTabModel
{
    Q_OBJECT

protected:
    virtual void createTab(const Tab &tab);
    virtual void updateTitle(int tabId, const QString &url, const QString &title);
    virtual void removeTab(int tabId);
    virtual void updateRequestedUrl(int tabId, const QString &requestedUrl, const QString &resolvedUrl);
    virtual void navigateTo(int tabId, const QString &url, const QString &title, const QString &path);
    virtual void updateThumbPath(int tabId, const QString &path);

public:
    PrivateTabModel(int nextTabId, DeclarativeWebContainer *webContainer = 0);
    ~PrivateTabModel();
};

#endif // PRIVATETABMODEL_H
