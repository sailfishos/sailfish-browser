/****************************************************************************
**
** Copyright (C) 2016
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QObject>
#include <QStringList>
#include <QMap>

typedef QMap<QString, QString> StringMap;

class OpenSearchConfigs : public QObject {

public:
    static const StringMap getAvailableOpenSearchConfigs();
    static const QStringList getSearchEngineList();

private:
    OpenSearchConfigs(QObject* parent=0);
};
