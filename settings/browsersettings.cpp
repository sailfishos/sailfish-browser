/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QDir>
#include <QXmlStreamReader>
#include "browsersettings.h"

static const QString openSearchPath("/usr/lib/mozembedlite/chrome/embedlite/content/");

BrowserSettings::BrowserSettings(QObject *parent)
    : QObject(parent)
{
}

BrowserSettings::~BrowserSettings()
{
}

const QStringList BrowserSettings::getSearchEngineList() const
{
    QStringList engineList;
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
            engineList.append(searchEngine);
        }

        xmlFile.close();
    }

    return engineList;
}
