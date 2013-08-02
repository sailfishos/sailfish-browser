/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

#include <QDebug>
#include <QVariant>
#include "qmozcontext.h"
#include "settingmanager.h"

SettingManager::SettingManager(QObject *parent)
    : QObject(parent)
{
    m_clearPrivateDataConfItem = new MGConfItem("/apps/sailfish-browser/actions/clear_private_data", this);
}

void SettingManager::initialize()
{
    clearPrivateData();

    connect(m_clearPrivateDataConfItem, SIGNAL(valueChanged()),
            this, SLOT(clearPrivateData()));
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
