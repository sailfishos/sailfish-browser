/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

#include "dbmanager.h"

#include <QMetaObject>

#include "dbworker.h"

DBManager *DBManager::instance()
{
    static DBManager *dbManager;
    if (!dbManager)
        dbManager = new DBManager();
    return dbManager;
}

DBManager::DBManager(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<QList<Tab> >("QList<Tab>");
    qRegisterMetaType<QList<Link> >("QList<Link>");
    qRegisterMetaType<Tab>("Tab");

    worker = new DBWorker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(worker, SIGNAL(tabsAvailable(QList<Tab>)), this, SLOT(tabListAvailable(QList<Tab>)));
    connect(worker, SIGNAL(historyAvailable(QList<Link>)), this, SIGNAL(historyAvailable(QList<Link>)));
    connect(worker, SIGNAL(tabHistoryAvailable(int,QList<Link>)), this, SIGNAL(tabHistoryAvailable(int,QList<Link>)));
    connect(worker, SIGNAL(tabChanged(Tab)), this, SIGNAL(tabChanged(Tab)));
    connect(worker, SIGNAL(tabAvailable(Tab)), this, SIGNAL(tabAvailable(Tab)));
    connect(worker, SIGNAL(titleChanged(QString,QString)), this, SIGNAL(titleChanged(QString,QString)));
    connect(worker, SIGNAL(thumbPathChanged(QString,QString,int)), this, SIGNAL(thumbPathChanged(QString,QString,int)));
    workerThread.start();

    QMetaObject::invokeMethod(worker, "init", Qt::BlockingQueuedConnection);
    QMetaObject::invokeMethod(worker, "getMaxTabId", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(int, m_maxTabId));
    QMetaObject::invokeMethod(worker, "getSettings", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(SettingsMap, m_settings));
}

int DBManager::createTab()
{
    QMetaObject::invokeMethod(worker, "createTab", Qt::QueuedConnection, Q_ARG(int, ++m_maxTabId));
    return m_maxTabId;
}

void DBManager::getTab(int tabId)
{
    QMetaObject::invokeMethod(worker, "getTab", Qt::QueuedConnection,
                              Q_ARG(int, tabId));
}

void DBManager::navigateTo(int tabId, QString url, QString title, QString path)
{
    QMetaObject::invokeMethod(worker, "navigateTo", Qt::QueuedConnection,
                              Q_ARG(int, tabId), Q_ARG(QString, url),
                              Q_ARG(QString, title), Q_ARG(QString, path));
}

void DBManager::updateTab(int tabId, QString url, QString title, QString path)
{
    QMetaObject::invokeMethod(worker, "updateTab", Qt::QueuedConnection,
                              Q_ARG(int, tabId), Q_ARG(QString, url),
                              Q_ARG(QString, title), Q_ARG(QString, path));
}

void DBManager::goForward(int tabId)
{
    QMetaObject::invokeMethod(worker, "goForward", Qt::QueuedConnection,
                              Q_ARG(int, tabId));
}

void DBManager::goBack(int tabId)
{
    QMetaObject::invokeMethod(worker, "goBack", Qt::QueuedConnection,
                              Q_ARG(int, tabId));
}

void DBManager::getAllTabs()
{
    QMetaObject::invokeMethod(worker, "getAllTabs", Qt::QueuedConnection);
}

void DBManager::removeTab(int tabId)
{
    QMetaObject::invokeMethod(worker, "removeTab", Qt::QueuedConnection,
                              Q_ARG(int, tabId));
}

void DBManager::removeAllTabs()
{
    QMetaObject::invokeMethod(worker, "removeAllTabs", Qt::QueuedConnection);
}

void DBManager::updateTitle(QString url, QString title)
{
    QMetaObject::invokeMethod(worker, "updateTitle", Qt::QueuedConnection,
                              Q_ARG(QString, url), Q_ARG(QString, title));
}

void DBManager::updateThumbPath(QString url, QString path, int tabId)
{
    QMetaObject::invokeMethod(worker, "updateThumbPath", Qt::QueuedConnection,
                              Q_ARG(QString, url), Q_ARG(QString, path), Q_ARG(int, tabId));
}

void DBManager::clearHistory()
{
    QMetaObject::invokeMethod(worker, "clearHistory", Qt::QueuedConnection);
}

void DBManager::getHistory(const QString &filter)
{
    QMetaObject::invokeMethod(worker, "getHistory", Qt::QueuedConnection, Q_ARG(QString, filter));
}

void DBManager::clearTabHistory(int tabId)
{
    QMetaObject::invokeMethod(worker, "clearTabHistory", Qt::QueuedConnection, Q_ARG(int, tabId));
}

void DBManager::getTabHistory(int tabId)
{
    QMetaObject::invokeMethod(worker, "getTabHistory", Qt::QueuedConnection, Q_ARG(int, tabId));
}

void DBManager::saveSetting(QString name, QString value)
{
    m_settings.insert(name, value);
    emit settingsChanged();
    QMetaObject::invokeMethod(worker, "saveSetting", Qt::QueuedConnection,
                              Q_ARG(QString, name), Q_ARG(QString, value));
}

QString DBManager::getSetting(QString name)
{
    if (m_settings.contains(name)) {
        return m_settings.value(name);
    }

    return "";
}

void DBManager::deleteSetting(QString name)
{
    if (m_settings.contains(name)) {
        m_settings.remove(name);
        emit settingsChanged();
        QMetaObject::invokeMethod(worker, "deleteSetting", Qt::QueuedConnection,
                                  Q_ARG(QString, name));
    }
}

void DBManager::tabListAvailable(QList<Tab> tabs)
{
    emit tabsAvailable(tabs);
}
