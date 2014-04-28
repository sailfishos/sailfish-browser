/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "settingmanager.h"
#include "dbmanager.h"

#include <qmozcontext.h>
#include <QVariant>

SettingManager::SettingManager(QObject *parent)
    : QObject(parent)
{
    m_clearPrivateDataConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_private_data", this);
    m_searchEngineConfItem = new MGConfItem("/apps/sailfish-browser/settings/search_engine", this);
}

bool SettingManager::clearPrivateDataRequested() const
{
    return m_clearPrivateDataConfItem->value(QVariant(false)).toBool();
}

void SettingManager::initialize()
{
    clearPrivateData();
    setSearchEngine();

    connect(m_clearPrivateDataConfItem, SIGNAL(valueChanged()),
            this, SLOT(clearPrivateData()));
    connect(m_searchEngineConfItem, SIGNAL(valueChanged()),
            this, SLOT(setSearchEngine()));
}

void SettingManager::clearPrivateData()
{
    if (clearPrivateDataRequested()) {
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("passwords"));
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("cookies"));
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("cache"));
        DBManager::instance()->clearHistory();
        m_clearPrivateDataConfItem->set(QVariant(false));
    }
}

void SettingManager::setSearchEngine()
{
    QVariant searchEngine = m_searchEngineConfItem->value(QVariant(QString("Google")));
    QMozContext::GetInstance()->setPref(QString("browser.search.defaultenginename"), searchEngine);
}
