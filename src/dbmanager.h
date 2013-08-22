/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QMap>
#include <QThread>

#include "link.h"
#include "tab.h"

class DBWorker;

class DBManager : public QObject
{
    Q_OBJECT
public:
    static DBManager *instance();

    int createTab();
    void getTab(int tabId);
    void getAllTabs();
    void removeTab(int tabId);
    void removeAllTabs();
    void navigateTo(int tabId, QString url, QString title = "", QString path = "");
    void updateTab(int tabId, QString url, QString title = "", QString path = "");
    void goForward(int tabId);
    void goBack(int tabId);

    void updateThumbPath(QString url, QString path);
    void updateTitle(QString url, QString title);

    void clearHistory();
    void getHistory(const QString &filter = "");
    void clearTabHistory(int tabId);
    void getTabHistory(int tabId);

    void saveSetting(QString name, QString value);
    QString getSetting(QString name);
    void deleteSetting(QString name);

public slots:
    void tabListAvailable(QList<Tab> tabs);

signals:
    void tabChanged(Tab tab);
    void tabAvailable(Tab tab);
    void tabsAvailable(QList<Tab> tab);
    void historyAvailable(QList<Link> links);
    void tabHistoryAvailable(int tabId, QList<Link> links);
    void thumbPathChanged(QString url, QString path);
    void titleChanged(QString url, QString title);
    void settingsChanged();

private:
    DBManager(QObject *parent = 0);

    int getMaxTabId();

    int m_maxTabId;
    QMap<QString, QString> m_settings;

    QThread workerThread;
    DBWorker *worker;
};

#endif // DBMANAGER_H
