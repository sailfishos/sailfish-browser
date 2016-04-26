/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SETTINGMANAGER_H
#define SETTINGMANAGER_H

#include <QObject>

class MGConfItem;

class SettingManager : public QObject
{
    Q_OBJECT

public:
    static SettingManager *instance();

public slots:
    void setSearchEngine();

private:
    explicit SettingManager(QObject *parent = 0);
};

#endif
