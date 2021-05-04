/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef FAVICONMANAGER_H
#define FAVICONMANAGER_H

#include <QObject>
#include <QMap>

#include "declarativewebpage.h"

class FaviconManager : public QObject
{
    Q_OBJECT

public:
    static FaviconManager *instance();

    Q_INVOKABLE void add(const QString &type, const QString &hostname, const QString &favicon, bool hasTouchIcon);
    Q_INVOKABLE void remove(const QString &type, const QString &hostname);
    QString get(const QString &type, const QString &hostname);
    Q_INVOKABLE void grabIcon(const QString &type, DeclarativeWebPage *webPage, const QSize &size);

private:
    FaviconManager(QObject *parent = nullptr);

    void save(const QString &type);
    void load(const QString &type);

    struct Favicon {
        QString favicon;
        bool hasTouchIcon;
    };

    struct FaviconSet {
        bool loaded;
        QMap<QString, Favicon> favicons;
    };

    QMap<QString, FaviconSet> m_faviconSets;
};

#endif // FAVICONMANAGER_H
