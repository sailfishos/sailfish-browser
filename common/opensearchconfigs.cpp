/****************************************************************************
**
** Copyright (C) 2015 - 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include "opensearchconfigs.h"

OpenSearchConfigs *OpenSearchConfigs::openSearchConfigs = 0;

OpenSearchConfigs::OpenSearchConfigs(QObject *parent):QObject(parent)
{
    m_openSearchPathList << QString(EMBEDLITE_CONTENT_PATH);
    m_openSearchPathList << getOpenSearchConfigPath();
}

const StringMap OpenSearchConfigs::parseOpenSearchConfigs()
{
    StringMap configs;

    foreach(QString openSearchPath, m_openSearchPathList) {
        QDir configDir(openSearchPath);
        configDir.setSorting(QDir::Name);

        foreach (QString fileName, configDir.entryList(QStringList("*.xml"))) {
            QFile xmlFile(openSearchPath + fileName);
            xmlFile.open(QIODevice::ReadOnly | QIODevice::Text);
            QXmlStreamReader xml(&xmlFile);
            QString searchEngine;

            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement() && xml.name() == "ShortName") {
                    xml.readNext();
                    if (xml.isCharacters()) {
                        searchEngine = xml.text().toString();
                    }
                }
            }

            if (!xml.hasError()) {
                configs.insert(searchEngine, openSearchPath + fileName);
            }

            xmlFile.close();
        }
    }
    return configs;
}

OpenSearchConfigs* OpenSearchConfigs::getInstance()
{
    if (!openSearchConfigs) {
        openSearchConfigs = new OpenSearchConfigs();
    }
    return openSearchConfigs;
}

const QStringList OpenSearchConfigs::getSearchEngineList()
{
    // Return names of search engines
    return getInstance()->parseOpenSearchConfigs().keys();
}

const StringMap OpenSearchConfigs::getAvailableOpenSearchConfigs()
{
    return getInstance()->parseOpenSearchConfigs();
}

const QString OpenSearchConfigs::getOpenSearchConfigPath()
{
    return QDir::homePath() + USER_OPENSEARCH_PATH;
}
