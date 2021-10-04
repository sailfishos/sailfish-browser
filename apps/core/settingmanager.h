/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
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
    bool initialize();

    int toolbarSmall();
    int toolbarLarge();

    static SettingManager *instance();

    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void clearCookies();
    Q_INVOKABLE void clearPasswords();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void removeAllTabs();

signals:
    void toolbarSmallChanged();
    void toolbarLargeChanged();

private slots:
    void setSearchEngine();
    void handleObserve(const QString &message, const QVariant &data);

private:
    explicit SettingManager(QObject *parent = 0);

    MGConfItem *m_searchEngineConfItem;

    MGConfItem *m_toolbarSmall;
    MGConfItem *m_toolbarLarge;

    bool m_initialized;
    bool m_searchEnginesInitialized;

    QStringList *m_addedSearchEngines;
};

#endif
