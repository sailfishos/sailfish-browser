/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// This implementation is mock implementation for testing only.

#include "faviconmanager.h" // mock

#include <QUrl>

FaviconManager *FaviconManager::instance()
{
    static FaviconManager *mgr = new FaviconManager;
    return mgr;
}

QString FaviconManager::sanitizedHostname(const QString &hostname)
{
    return QUrl(hostname).host();
}

void FaviconManager::add(const QString &type, const QString &hostname, const QString &favicon, bool hasTouchIcon)
{
    m_faviconSets[type].favicons.insert(hostname, { favicon, hasTouchIcon });
}

void FaviconManager::remove(const QString &type, const QString &hostname)
{
    m_faviconSets[type].favicons.remove(hostname);
}

QString FaviconManager::get(const QString &type, const QString &hostname)
{
    return m_faviconSets.value(type).favicons.value(hostname).favicon;
}

void FaviconManager::grabIcon(const QString &, DeclarativeWebPage *, const QSize &)
{
}

void FaviconManager::clear(const QString &type)
{
    m_faviconSets.remove(type);
}

FaviconManager::FaviconManager(QObject *parent)
    : QObject(parent)
{
}

void FaviconManager::save(const QString &)
{
}

void FaviconManager::load(const QString &)
{
}
