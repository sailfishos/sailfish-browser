/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#ifndef DBWORKER_H
#define DBWORKER_H

#include <QObject>
#include <QMap>
#include <QSqlDatabase>

#include "link.h"
#include "tab.h"

// Typedefs are necessary because of use of Q_RETURN_ARG, which does understand
// comma-separated types
typedef QMap<QString, QString> SettingsMap;

class DBWorker : public QObject
{
    Q_OBJECT
public:
    DBWorker(QObject *parent = 0);

public slots:
    void init();
    void createTab(int tabId);
    void removeTab(int tabId);
    void removeAllTabs();
    void getTab(int tabId);
    void getAllTabs();
    void navigateTo(int tabId, QString url, QString title, QString path);
    void updateTab(int tabId, QString url, QString title, QString path);
    int getMaxTabId();

    void updateTitle(QString url, QString title);
    void updateThumbPath(QString url, QString path);

    void goForward(int tabId);
    void goBack(int tabId);
    void getHistory();
    void getTabHistory(int tabId);
    void clearHistory();
    void clearTabHistory(int tabId);

    void saveSetting(QString name, QString value);
    SettingsMap getSettings();
    void deleteSetting(QString name);

signals:
    void tabAvailable(Tab tab);
    void tabChanged(Tab tab);
    void tabsAvailable(QList<Tab> tabs);
    void thumbPathChanged(QString url, QString path);
    void titleChanged(QString url, QString title);
    void tabHistoryAvailable(int tabId, QList<Link>);
    void historyAvailable(QList<Link>);
    void error(QString query);

private:
    Link getLink(int linkId);
    Link getLink(QString url);
    void updateLink(int linkId, QString url, QString title, QString thumbPath);
    bool addToHistory(int linkId);
    int addToTabHistory(int tabId, int linkId);
    Link getLinkFromTabHistory(int tabHistoryId);
    Link getCurrentLink(int tabId);
    int getNextLinkIdFromTabHistory(int tabHistoryId);
    int getPreviousLinkIdFromTabHistory(int tabHistoryId);
    int createLink(QString url, QString title = "", QString thumbPath = "");
    bool updateTab(int tabId, int tabHistoryId);
    Tab getTabData(int tabId, int historyId = 0);

    QSqlQuery prepare(const char* statement);
    bool execute(QSqlQuery &query);
    QSqlDatabase m_database;
};

#endif // DBWORKER_H
