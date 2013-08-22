/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

#include <QVariant>
#include "qmozcontext.h"
#include "settingmanager.h"

SettingManager::SettingManager(QObject *parent)
    : QObject(parent)
{
    m_clearPrivateDataConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_private_data", this);
    m_searchEngineConfItem = new MGConfItem("/apps/sailfish-browser/settings/search_engine", this);
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
    bool actionNeeded = m_clearPrivateDataConfItem->value(QVariant(false)).toBool();

    if (actionNeeded) {
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("passwords"));
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("cookies"));
        QMozContext::GetInstance()->sendObserve(QString("clear-private-data"), QString("cache"));
        m_clearPrivateDataConfItem->set(QVariant(false));
    }
}

void SettingManager::setSearchEngine()
{
    QVariant searchEngine = m_searchEngineConfItem->value(QVariant(QString("Google")));
    QMozContext::GetInstance()->setPref(QString("browser.search.defaultenginename"), searchEngine);
}
