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
#include <QVariant>

#include <webengine.h>
#include <webenginesettings.h>

static SettingManager *gSingleton = 0;

SettingManager::SettingManager(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
    m_clearHistoryConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_history", this);
    m_clearCookiesConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_cookies", this);
    m_clearPasswordsConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_passwords", this);
    m_clearCacheConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_cache", this);
    m_doNotTrackConfItem = new MGConfItem("/apps/sailfish-browser/settings/do_not_track", this);
    m_autostartPrivateBrowsing = new MGConfItem("/apps/sailfish-browser/settings/autostart_private_browsing", this);

    // Look and feel related settings
    m_toolbarSmall = new MGConfItem("/apps/sailfish-browser/settings/toolbar_small", this);
    m_toolbarLarge = new MGConfItem("/apps/sailfish-browser/settings/toolbar_large", this);
    connect(m_toolbarSmall, &MGConfItem::valueChanged, this, &SettingManager::toolbarSmallChanged);
    connect(m_toolbarLarge, &MGConfItem::valueChanged, this, &SettingManager::toolbarLargeChanged);
}

bool SettingManager::clearHistoryRequested() const
{
    return m_clearHistoryConfItem->value(QVariant(false)).toBool();
}

bool SettingManager::initialize()
{
    if (m_initialized) {
        return false;
    }

    bool clearedData = clearCookies();
    clearedData |= clearPasswords();
    clearedData |= clearCache();
    clearedData |= clearHistory();

    doNotTrack();

    connect(m_clearHistoryConfItem, &MGConfItem::valueChanged,
            this, &SettingManager::clearHistory);
    connect(m_clearCookiesConfItem, &MGConfItem::valueChanged,
            this, &SettingManager::clearCookies);
    connect(m_clearPasswordsConfItem, &MGConfItem::valueChanged,
            this, &SettingManager::clearPasswords);
    connect(m_clearCacheConfItem, &MGConfItem::valueChanged,
            this, &SettingManager::clearCache);
    connect(m_doNotTrackConfItem, &MGConfItem::valueChanged,
            this, &SettingManager::doNotTrack);

    m_initialized = true;
    return clearedData;
}

int SettingManager::toolbarSmall()
{
    return m_toolbarSmall->value(72).value<int>();
}

int SettingManager::toolbarLarge()
{
    return m_toolbarLarge->value(108).value<int>();
}

SettingManager *SettingManager::instance()
{
    if (!gSingleton) {
        gSingleton = new SettingManager();
    }
    return gSingleton;
}

bool SettingManager::clearHistory()
{
    bool actionNeeded = m_clearHistoryConfItem->value(false).toBool();
    if (actionNeeded) {
        DBManager::instance()->clearHistory();
        m_clearHistoryConfItem->set(false);
    }
    return actionNeeded;
}

bool SettingManager::clearCookies()
{
    bool actionNeeded = m_clearCookiesConfItem->value(false).toBool();
    if (actionNeeded) {
        SailfishOS::WebEngine::instance()->notifyObservers(QString("clear-private-data"), QString("cookies"));
        m_clearCookiesConfItem->set(false);
    }
    return actionNeeded;
}

bool SettingManager::clearPasswords()
{
    bool actionNeeded = m_clearPasswordsConfItem->value(false).toBool();
    if (actionNeeded) {
        SailfishOS::WebEngine::instance()->notifyObservers(QString("clear-private-data"), QString("passwords"));
        m_clearPasswordsConfItem->set(false);
    }
    return actionNeeded;
}

bool SettingManager::clearCache()
{
    bool actionNeeded = m_clearCacheConfItem->value(false).toBool();
    if (actionNeeded) {
        SailfishOS::WebEngine::instance()->notifyObservers(QString("clear-private-data"), QString("cache"));
        m_clearCacheConfItem->set(false);
    }
    return actionNeeded;
}

void SettingManager::doNotTrack()
{
    SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();
    webEngineSettings->setPreference(QString("privacy.donottrackheader.enabled"),
                                     m_doNotTrackConfItem->value(false));
}

void SettingManager::setAutostartPrivateBrowsing(bool privateMode)
{
    m_autostartPrivateBrowsing->set(privateMode);
}

bool SettingManager::autostartPrivateBrowsing() const
{
    return m_autostartPrivateBrowsing->value(false).toBool();
}
