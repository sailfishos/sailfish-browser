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
#include "opensearchconfigs.h"

#include <MGConfItem>
#include <QVariant>

#include <webengine.h>
#include <webenginesettings.h>

static SettingManager *gSingleton = 0;

SettingManager::SettingManager(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_searchEnginesInitialized(false)
    , m_addedSearchEngines(0)
{
    m_clearHistoryConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_history", this);
    m_clearCookiesConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_cookies", this);
    m_clearPasswordsConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_passwords", this);
    m_clearCacheConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_cache", this);
    m_searchEngineConfItem = new MGConfItem("/apps/sailfish-browser/settings/search_engine", this);
    m_doNotTrackConfItem = new MGConfItem("/apps/sailfish-browser/settings/do_not_track", this);
    m_autostartPrivateBrowsing = new MGConfItem("/apps/sailfish-browser/settings/autostart_private_browsing", this);

    // Look and feel related settings
    m_toolbarSmall = new MGConfItem("/apps/sailfish-browser/settings/toolbar_small", this);
    m_toolbarLarge = new MGConfItem("/apps/sailfish-browser/settings/toolbar_large", this);
    connect(m_toolbarSmall, SIGNAL(valueChanged()), this, SIGNAL(toolbarSmallChanged()));
    connect(m_toolbarLarge, SIGNAL(valueChanged()), this, SIGNAL(toolbarLargeChanged()));
    connect(SailfishOS::WebEngine::instance(), SIGNAL(recvObserve(QString, QVariant)),
            this, SLOT(handleObserve(QString, QVariant)));
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

    setSearchEngine();
    doNotTrack();

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

void SettingManager::setSearchEngine()
{
    if (m_searchEnginesInitialized) {
        QVariant searchEngine = m_searchEngineConfItem->value(QVariant(QString("Google")));
        SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();
        webEngineSettings->setPreference(QString("browser.search.defaultenginename"), searchEngine);

        // Let nsSearchService update the search engine (through EmbedLiteSearchEngine).
        QVariantMap defaultSearchEngine;
        defaultSearchEngine.insert(QLatin1String("msg"), QLatin1String("setdefault"));
        defaultSearchEngine.insert(QLatin1String("name"), searchEngine);
        SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();
        webEngine->notifyObservers(QLatin1String("embedui:search"), QVariant(defaultSearchEngine));
    }
}

void SettingManager::doNotTrack()
{
    SailfishOS::WebEngineSettings *webEngineSettings = SailfishOS::WebEngineSettings::instance();
    webEngineSettings->setPreference(QString("privacy.donottrackheader.enabled"),
                                     m_doNotTrackConfItem->value(false));
}

void SettingManager::handleObserve(const QString &message, const QVariant &data)
{
    const QVariantMap dataMap = data.toMap();
    if (message == QLatin1String("embed:search")) {
        QString msg = dataMap.value("msg").toString();
        if (msg == QLatin1String("init")) {
            const StringMap configs(OpenSearchConfigs::getAvailableOpenSearchConfigs());
            const QStringList configuredEngines = configs.keys();
            QStringList registeredSearches(dataMap.value(QLatin1String("engines")).toStringList());
            QString defaultSearchEngine = dataMap.value(QLatin1String("defaultEngine")).toString();
            m_searchEnginesInitialized = !registeredSearches.isEmpty();

            // Upon first start, engine doesn't know about the search engines.
            // Engine load requests are send within the for loop below.
            if (!m_searchEnginesInitialized) {
                m_addedSearchEngines = new QStringList(configuredEngines);
            }

            SailfishOS::WebEngine *webEngine = SailfishOS::WebEngine::instance();

            // Add newly installed configs
            foreach (QString searchName, configuredEngines) {
                if (registeredSearches.contains(searchName)) {
                    registeredSearches.removeAll(searchName);
                } else {
                    QVariantMap loadsearch;
                    // load opensearch descriptions
                    loadsearch.insert(QLatin1String("msg"), QVariant(QLatin1String("loadxml")));
                    loadsearch.insert(QLatin1String("uri"), QVariant(QString("file://%1").arg(configs[searchName])));
                    loadsearch.insert(QLatin1String("confirm"), QVariant(false));
                    webEngine->notifyObservers(QLatin1String("embedui:search"), QVariant(loadsearch));
                }
            }

            // Remove uninstalled OpenSearch configs
            foreach(QString searchName, registeredSearches) {
                QVariantMap removeMsg;
                removeMsg.insert(QLatin1String("msg"), QVariant(QLatin1String("remove")));
                removeMsg.insert(QLatin1String("name"), QVariant(searchName));
                webEngine->notifyObservers(QLatin1String("embedui:search"), QVariant(removeMsg));
            }

            // Try to set search engine. After first start we can update the default search
            // engine immediately.
            setSearchEngine();
        } else if (msg == QLatin1String("search-engine-added")) {
            // We're only interrested about the very first start. Then the m_addedSearchEngines
            // contains engines.
            int errorCode = dataMap.value(QLatin1String("errorCode")).toInt();
            bool firstStart = m_addedSearchEngines && !m_addedSearchEngines->isEmpty();
            if (errorCode != 0) {
                qWarning() << "An error occurred while adding a search engine, error code: " << errorCode << ", see nsIBrowserSearchService for more details.";
            } else if (m_addedSearchEngines) {
                QString engine = dataMap.value(QLatin1String("engine")).toString();
                m_addedSearchEngines->removeAll(engine);
                m_searchEnginesInitialized = m_addedSearchEngines->isEmpty();
                // All engines are added.
                if (firstStart && m_searchEnginesInitialized) {
                    setSearchEngine();
                    delete m_addedSearchEngines;
                    m_addedSearchEngines = 0;
                }
            }
        }
    }
}

void SettingManager::setAutostartPrivateBrowsing(bool privateMode)
{
    m_autostartPrivateBrowsing->set(privateMode);
}

bool SettingManager::autostartPrivateBrowsing() const
{
    return m_autostartPrivateBrowsing->value(false).toBool();
}
