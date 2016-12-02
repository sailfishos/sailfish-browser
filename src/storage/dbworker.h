/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DBWORKER_H
#define DBWORKER_H

#include <QObject>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "link.h"
#include "tab.h"

// Typedefs are necessary because of use of Q_RETURN_ARG, which does understand
// comma-separated types
typedef QMap<QString, QString> SettingsMap;

enum HistoryResult { Error, Added, Skipped };

class DBWorker : public QObject
{
    Q_OBJECT

public:
    DBWorker(QObject *parent = 0);

public slots:
    void init();
    void createTab(const Tab &tab);
    void removeTab(int tabId);
    void getAllTabs();
    void removeAllTabs(bool noFeedback = false);
    void navigateTo(int tabId, QString url, QString title, QString path);
    int getMaxTabId();

    void updateTitle(int tabId, QString url, QString title);
    void updateThumbPath(int tabId, QString path);

    void goForward(int tabId);
    void goBack(int tabId);
    void getHistory(const QString &filter);
    void getTabHistory(int tabId);

    void removeHistoryEntry(int linkId);
    void clearHistory();

    void saveSetting(QString name, QString value);
    SettingsMap getSettings();
    void deleteSetting(QString name);

signals:
    void tabsAvailable(QList<Tab> tabs);
    void thumbPathChanged(int tabId, QString path);
    void titleChanged(QString url, QString title);
    void tabHistoryAvailable(int tabId, QList<Link>, int currentLinkId);
    void historyAvailable(QList<Link>);
    void error(QString query);

private:
    HistoryResult addToBrowserHistory(QString url, QString title);
    int addToTabHistory(int tabId, int linkId);
    Link getCurrentLink(int tabId);
    void clearDeprecatedTabHistory(int tabId, int currentLinkId);
    int createLink(QString url, QString title = "", QString thumbPath = "");
    void updateTab(int tabId, int tabHistoryId);
    int tabCount();
    int integerQuery(const QString &statement);
    void migrateTo_1();
    void setUserVersion(int userVersion);

    QSqlQuery prepare(const QString &statement);
    bool execute(QSqlQuery &query);
    QSqlDatabase m_database;
    QSqlQuery m_updateThumbPathQuery;
};

#endif // DBWORKER_H
