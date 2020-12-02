/****************************************************************************
**
** Copyright (c) 2013 - 2014 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BROWSERSETTINGS_H
#define BROWSERSETTINGS_H

#include <QObject>
#include <QStringList>

class BrowserSettings: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList searchEngineList READ getSearchEngineList CONSTANT)

public:
    explicit BrowserSettings(QObject *parent = 0);
    virtual ~BrowserSettings();

    const QStringList getSearchEngineList() const;
};

#endif
