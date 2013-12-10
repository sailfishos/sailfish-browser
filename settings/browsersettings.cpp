/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

#include "browsersettings.h"

BrowserSettings::BrowserSettings(QObject *parent)
    : QObject(parent)
{
}

BrowserSettings::~BrowserSettings()
{
}

const QString BrowserSettings::testProp() const
{
    return QString("test property");
}
