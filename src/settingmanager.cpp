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

#include <MGConfItem>
#include <qmozcontext.h>
#include <QVariant>

SettingManager::SettingManager(QObject *parent)
    : QObject(parent)
{
    m_clearPrivateDataConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_private_data", this);
    m_clearHistoryConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_history", this);
    m_clearCookiesConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_cookies", this);
    m_clearPasswordsConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_passwords", this);
    m_clearCacheConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_cache", this);
    m_searchEngineConfItem = new MGConfItem("/apps/sailfish-browser/settings/search_engine", this);
    m_doNotTrackConfItem = new MGConfItem("/apps/sailfish-browser/settings/do_not_track", this);
}

bool SettingManager::clearHistoryRequested() const
{
    return m_clearPrivateDataConfItem->value(QVariant(false)).toBool() ||
            m_clearHistoryConfItem->value(QVariant(false)).toBool();
}

void SettingManager::initialize()
{
    if (!clearPrivateData()) {
        clearHistory();
        clearCookies();
        clearPasswords();
        clearCache();
    }
    setSearchEngine();
    doNotTrack();

    connect(m_clearPrivateDataConfItem, SIGNAL(valueChanged()),
            this, SLOT(clearPrivateData()));
    connect(m_clearHistoryConfItem, SIGNAL(valueChanged()),
            this, SLOT(clearHistory()));
    connect(m_clearCookiesConfItem, SIGNAL(valueChanged()),
            this, SLOT(clearCookies()));
    connect(m_clearPasswordsConfItem, SIGNAL(valueChanged()),
            this, SLOT(clearPasswords()));
    connect(m_clearCacheConfItem, SIGNAL(valueChanged()),
            this, SLOT(clearCache()));
    connect(m_searchEngineConfItem, SIGNAL(valueChanged()),
            this, SLOT(setSearchEngine()));
    connect(m_doNotTrackConfItem, SIGNAL(valueChanged()),
            this, SLOT(doNotTrack()));
}

bool SettingManager::clearPrivateData()
{
    bool actionNeeded = m_clearPrivateDataConfItem->value(QVariant(false)).toBool();
    if (actionNeeded) {
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("passwords"));
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("cookies"));
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("cache"));
        DBManager::instance()->clearHistory();
        m_clearPrivateDataConfItem->set(QVariant(false));
    }
    return actionNeeded;
}

void SettingManager::clearHistory()
{
    bool actionNeeded = m_clearHistoryConfItem->value(false).toBool();
    if (actionNeeded) {
        DBManager::instance()->clearHistory();
        m_clearHistoryConfItem->set(false);
    }
}

void SettingManager::clearCookies()
{
    bool actionNeeded = m_clearCookiesConfItem->value(false).toBool();
    if (actionNeeded) {
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("cookies"));
        m_clearCookiesConfItem->set(false);
    }
}

void SettingManager::clearPasswords()
{
    bool actionNeeded = m_clearPasswordsConfItem->value(false).toBool();
    if (actionNeeded) {
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("passwords"));
        m_clearPasswordsConfItem->set(false);
    }
}

void SettingManager::clearCache()
{
    bool actionNeeded = m_clearCacheConfItem->value(false).toBool();
    if (actionNeeded) {
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("cache"));
        m_clearCacheConfItem->set(false);
    }
}

void SettingManager::setSearchEngine()
{
    QVariant searchEngine = m_searchEngineConfItem->value(QVariant(QString("Google")));
    QMozContext::GetInstance()->setPref(QString("browser.search.defaultenginename"), searchEngine);
}

void SettingManager::doNotTrack()
{
    QMozContext::GetInstance()->setPref(QString("privacy.donottrackheader.enabled"),
                                        m_doNotTrackConfItem->value(false));
}
