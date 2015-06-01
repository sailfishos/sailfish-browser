/****************************************************************************
**
** Copyright (C) 2015
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <QStringList>
#include <QMap>

typedef QMap<QString, QString> StringMap;

class OpenSearchConfigs : public QObject {

public:
    static const StringMap getAvailableOpenSearchConfigs();
    static const QStringList getSearchEngineList();

private:
    QStringList m_openSearchPathList;

    static OpenSearchConfigs* getInstance();
    static OpenSearchConfigs *openSearchConfigs;
    const StringMap parseOpenSearchConfigs();
    const QStringList parseSearchEngineList();
    OpenSearchConfigs(QObject* parent=0);
};
