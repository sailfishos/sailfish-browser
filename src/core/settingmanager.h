/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SETTINGMANAGER_H
#define SETTINGMANAGER_H

#include <QObject>

class MGConfItem;

class SettingManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int toolbarSmall READ toolbarSmall NOTIFY toolbarSmallChanged FINAL)
    Q_PROPERTY(int toolbarLarge READ toolbarLarge NOTIFY toolbarLargeChanged FINAL)

public:
    bool clearHistoryRequested() const;
    bool initialize();

    int toolbarSmall();
    int toolbarLarge();

    bool autostartPrivateBrowsing() const;
    void setAutostartPrivateBrowsing(bool);

    static SettingManager *instance();

public slots:
    void setSearchEngine();

signals:
    void toolbarSmallChanged();
    void toolbarLargeChanged();

private slots:
    bool clearHistory();
    bool clearCookies();
    bool clearPasswords();
    bool clearCache();
    void doNotTrack();

private:
    explicit SettingManager(QObject *parent = 0);

    MGConfItem *m_clearHistoryConfItem;
    MGConfItem *m_clearCookiesConfItem;
    MGConfItem *m_clearPasswordsConfItem;
    MGConfItem *m_clearCacheConfItem;
    MGConfItem *m_searchEngineConfItem;
    MGConfItem *m_doNotTrackConfItem;
    MGConfItem *m_autostartPrivateBrowsing;

    MGConfItem *m_toolbarSmall;
    MGConfItem *m_toolbarLarge;

    bool m_initialized;
};

#endif
