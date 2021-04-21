/****************************************************************************
**
** Copyright (c) 2016 - 2021 Jolla Ltd.
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
    static const QString getOpenSearchConfigPath();

private:
    OpenSearchConfigs(QObject* parent=0);
};
