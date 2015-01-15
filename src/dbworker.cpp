/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dbworker.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlResult>
#include <QSqlDriver>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QDateTime>

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

#define DB_USER_VERSION 1

#define QUOTE(arg) #arg
#define STR(arg) QUOTE(arg)

static const char * const create_table_tab =
        "CREATE TABLE tab (tab_id INTEGER PRIMARY KEY,\n"
        "tab_history_id INTEGER\n"
        ");\n";

static const char * const create_table_link =
        "CREATE TABLE link (link_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "url TEXT,\n"
        "title TEXT,\n"
        "thumb_path TEXT\n"
        ");\n";

static const char * const create_table_browser_history =
        "CREATE TABLE browser_history (id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "url TEXT UNIQUE,\n"
        "title TEXT,\n"
        "favorite_icon TEXT,\n"
        "visited_count INTEGER DEFAULT 1,\n"
        "date INTEGER"
        ");\n";

static const char * const create_table_tab_history =
        "CREATE TABLE tab_history (id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "tab_id INTEGER,\n"
        "link_id INTEGER,\n"
        "date INT"
        ");\n";

static const char * const create_table_settings =
        "CREATE TABLE settings (name TEXT PRIMARY KEY,\n"
        "value TEXT\n"
        ");\n";

static const char * const set_user_version =
        "PRAGMA user_version=" STR(DB_USER_VERSION) ";\n";

static const char *db_schema[] = {
    create_table_tab,
    create_table_tab_history,
    create_table_link,
    create_table_browser_history,
    create_table_settings,
    set_user_version
};
static int db_schema_count = sizeof(db_schema) / sizeof(*db_schema);

DBWorker::DBWorker(QObject *parent) :
    QObject(parent)
{
}

void DBWorker::init()
{
    QString databaseDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString dbFileName = QLatin1String(DB_NAME);
    QDir dir(databaseDir);

    if(!dir.mkpath(databaseDir)) {
        qWarning() << "Can't create directory "+ databaseDir;
        return;
    }

    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dir.absoluteFilePath(dbFileName));
    bool dbCreated = dir.exists(dbFileName);
    bool ok = m_database.open();

    if (!ok)
        qWarning() << "Failed to open database " << m_database.databaseName();

    if (!dbCreated) {
        //TODO check transaction & rollback/commit
        for (int i = 0; i < db_schema_count; ++i) {
            QSqlQuery query = prepare(db_schema[i]);
            execute(query);
        }
    }

    // check current schema version and migrate if needed
    QSqlQuery schemaQuery = prepare("PRAGMA user_version;");
    if (execute(schemaQuery) && schemaQuery.next()) {
        int userVersion = schemaQuery.value(0).toInt();
        if (userVersion == 0) {
            migrateTo_1();
        }
    } else {
        qWarning() << "Failed to check schema version";
    }

    m_updateThumbPathQuery = prepare("UPDATE link SET thumb_path = ? "
                                     "WHERE link_id IN (SELECT link.link_id "
                                     "FROM tab_history INNER JOIN link ON tab_history.link_id=link.link_id WHERE tab_history.tab_id = ?);");
}

void DBWorker::setUserVersion(int userVersion)
{
    QSqlQuery updateQuery = prepare(QString("PRAGMA user_version = %1;").arg(userVersion));
    if (!execute(updateQuery)) {
        qWarning() << "Failed to update schema user version";
    }
}

// This method migrates data from history table (introduced in 42dbd01d23bc90cf1f5e177ceeefc05c91aa19cd) to browser_history table
void DBWorker::migrateTo_1() {
    // Check if browser_history table exists
    QSqlQuery browser_history_table_exists = prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='browser_history';");
    if(execute(browser_history_table_exists)) {
        if (!browser_history_table_exists.first()) {
            // browser_history table does not exist, let's create it
            browser_history_table_exists.clear();
            QSqlQuery create_browser_history_table = prepare(create_table_browser_history);
            if (!execute(create_browser_history_table)) {
                qCritical() << "Failed to create browser_history table";
            }
        }
    } else {
        qCritical() << "Failed to query for browser_history table";
    }

    QSqlQuery history_table_exists = prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='history';");
    if(execute(history_table_exists)) {
        if (history_table_exists.first()) {
            // history table exists, migrate all it's data to browser_history table and delete it
            history_table_exists.clear();

            QSqlQuery update_browser_history = prepare("INSERT INTO browser_history (url, title, date) select "\
                                               "link.url, link.title, history.date from link, history where "\
                                               "history.link_id = link.link_id and NULLIF(link.title, '') IS NOT NULL and "\
                                               "link.link_id in (select MAX(link_id) from link group by url);");


            if (!execute(update_browser_history)) {
                qCritical() << "Failed to update browser history";
            }

            QSqlQuery delete_history_table = prepare("DROP TABLE history;");
            if (!execute(delete_history_table)) {
                qCritical() << "Failed to delete history table";
            }
        }
    } else {
        qCritical() << "Failed to query for history table";
    }

    setUserVersion(1);
}

QSqlQuery DBWorker::prepare(const QString &statement)
{
    QSqlQuery query(m_database);
    query.setForwardOnly(true);
    if (!query.prepare(statement)) {
        qWarning() << Q_FUNC_INFO << "failed to prepare query";
        qWarning() << query.lastQuery();
        qWarning() << query.lastError();
        return QSqlQuery();
    }
    return query;
}

bool DBWorker::execute(QSqlQuery &query)
{
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "failed execute query";
        qWarning() << query.lastQuery();
        qWarning() << query.lastError();
        return false;
    }
    return true;
}

void DBWorker::createTab(int tabId)
{
#if DEBUG_LOGS
    qDebug() << "new tab id: " << tabId;
#endif
    QSqlQuery query = prepare("INSERT INTO tab (tab_id, tab_history_id) VALUES (?,?);");
    query.bindValue(0, tabId);
    query.bindValue(1, 0);
    execute(query);
}

int DBWorker::createLink(int tabId, QString url, QString title)
{
    if (url.isEmpty()) {
        return 0;
    }

    int linkId = createLink(url, title, "");

    if (addToBrowserHistory(url, title) == Error) {
        qWarning() << Q_FUNC_INFO << "failed to add url to history" << url;
    }

    int historyId = addToTabHistory(tabId, linkId);
    if (historyId > 0) {
        updateTab(tabId, historyId);
    } else {
        qWarning() << Q_FUNC_INFO << "failed to add url to tab history" << url;
    }

#if DEBUG_LOGS
    qDebug() << "created link:" << linkId << "with history id:" << historyId << "for tab:" << tabId << url;
#endif
    return linkId;
}

bool DBWorker::updateTab(int tabId, int tabHistoryId)
{
#if DEBUG_LOGS
    qDebug() << "tab:" << tabId << "tab history id:" << tabHistoryId;
#endif
    QSqlQuery query = prepare("UPDATE tab SET tab_history_id = ? WHERE tab_id = ?;");
    query.bindValue(0, tabHistoryId);
    query.bindValue(1, tabId);
    return execute(query);
}

Tab DBWorker::getTabData(int tabId, int historyId)
{
    int hId = historyId;
    if (historyId == 0) {
        QSqlQuery query = prepare("SELECT tab_history_id FROM tab WHERE tab_id = ?;");
        query.bindValue(0, tabId);
        if (execute(query)) {
            if (query.first()) {
                hId = query.value(0).toInt();
            }
        } else {
            return Tab();
        }
    }

    Link link = getLinkFromTabHistory(hId);
    int nextId = getNextLinkIdFromTabHistory(hId);
    int previousId = getPreviousLinkIdFromTabHistory(hId);
#if DEBUG_LOGS
    qDebug() << tabId << historyId << "next link id:" << nextId << "previous link id:" << previousId << link.linkId()<< link.title() << link.url();
#endif
    return Tab(tabId, link, nextId, previousId);
}

void DBWorker::removeTab(int tabId)
{
#if DEBUG_LOGS
    qDebug() << "tab id:" << tabId;
#endif
    QSqlQuery query = prepare("DELETE FROM tab WHERE tab_id = ?;");
    query.bindValue(0, tabId);
    execute(query);

    // Remove links that are only related to this tab
    query = prepare("DELETE FROM link WHERE link_id IN "
                    "(SELECT DISTINCT link_id FROM tab_history WHERE tab_id = ? "
                    "AND link_id NOT IN (SELECT link_id FROM tab_history WHERE tab_id != ? "
                    "))");
    query.bindValue(0, tabId);
    query.bindValue(1, tabId);

    // Remove history
    query = prepare("DELETE FROM tab_history WHERE tab_id = ?;");
    query.bindValue(0, tabId);
    execute(query);

    // Check last tab closed
    if (!tabCount()) {
        QList<Tab> tabList;
        emit tabsAvailable(tabList);
    }
}

void DBWorker::removeAllTabs()
{
    int oldTabCount = tabCount();
    QSqlQuery query = prepare("DELETE FROM tab;");
    execute(query);

    // Remove links that are not stored in history
    query = prepare("DELETE FROM link WHERE link_id IN "
                    "(SELECT DISTINCT link_id FROM tab_history)");
    execute(query);

    // Remove history
    query = prepare("DELETE FROM tab_history;");
    execute(query);

    QList<Tab> tabList;
    if (oldTabCount != 0) {
        emit tabsAvailable(tabList);
    }
}

void DBWorker::getTab(int tabId)
{
    QSqlQuery query = prepare("SELECT tab_id, tab_history_id FROM tab WHERE tab_id = ?;");
    query.bindValue(0, tabId);
    if (!execute(query)) {
        return;
    }

    if (query.first()) {
#if DEBUG_LOGS
        Tab tab = getTabData(query.value(0).toInt(), query.value(1).toInt());
        qDebug() << query.value(0).toInt() << query.value(1).toInt() << tab.title() << tab.url();
#endif
        emit tabAvailable(getTabData(query.value(0).toInt(), query.value(1).toInt()));
    }
}

void DBWorker::getAllTabs()
{
    QList<Tab> tabList;
    QSqlQuery query = prepare("SELECT tab_id, tab_history_id FROM tab;");
    if (!execute(query)) {
        return;
    }

    while (query.next()) {
        tabList.append(getTabData(query.value(0).toInt(), query.value(1).toInt()));
    }
    emit tabsAvailable(tabList);
}

int DBWorker::getMaxTabId()
{
    return integerQuery("SELECT MAX(tab_id) FROM tab;");
}

int DBWorker::getMaxLinkId()
{
    return integerQuery("SELECT MAX(link_id) FROM link;");
}

int DBWorker::tabCount()
{
    return integerQuery("SELECT COUNT(*) FROM tab;");
}

int DBWorker::integerQuery(const QString &statement)
{
    QSqlQuery query = prepare(statement);
    if (execute(query)) {
        if (query.first()) {
            return query.value(0).toInt();
        }
    }
    return 0;
}

void DBWorker::navigateTo(int tabId, QString url, QString title, QString path) {
    // TODO: rollback in case of failure
    if (url.isEmpty()) {
        return;
    }

    // Return if the current url of the tab is the same as the parameter url
    Link currentLink = getCurrentLink(tabId);
    if (currentLink.isValid() && currentLink.url() == url) {
        return;
    }

    clearDeprecatedTabHistory(tabId, currentLink.linkId());

    int linkId = createLink(url, title, path);

    if (addToBrowserHistory(url, title) == Error) {
        qWarning() << Q_FUNC_INFO << "failed to add url to history" << url;
    }

    int historyId = addToTabHistory(tabId, linkId);
    if (historyId > 0) {
        updateTab(tabId, historyId);
    } else {
        qWarning() << Q_FUNC_INFO << "failed to add url to tab history" << url;
    }

#if DEBUG_LOGS
    qDebug() << "emit tab changed:" << tabId << historyId << title << url;
#endif
}

void DBWorker::updateTab(int tabId, QString url, QString title, QString path)
{
    Link currentLink = getCurrentLink(tabId);
    if (!currentLink.isValid()) {
        qWarning() << "attempt to update url that is not stored in db." << tabId << title << url << path << currentLink.linkId() << currentLink.url();
        return;
    }
#if DEBUG_LOGS
    qDebug() << tabId << title << url << path;
#endif
    updateLink(currentLink.linkId(), url, title, path);
}

void DBWorker::goForward(int tabId) {
    QSqlQuery query = prepare("SELECT id FROM tab_history WHERE tab_id = ? AND id > (SELECT tab_history_id FROM tab WHERE tab_id = ?) ORDER BY id ASC LIMIT 1;");
    query.bindValue(0, tabId);
    query.bindValue(1, tabId);
    if (!execute(query)) {
        return;
    }

    int historyId = 0;
    if (query.first()) {
        historyId = query.value(0).toInt();
    }

    if (historyId > 0) {
        if (updateTab(tabId, historyId)) {
            emit tabChanged(getTabData(tabId, historyId));
        }
    }
}

void DBWorker::goBack(int tabId) {
    QSqlQuery query = prepare("SELECT id FROM tab_history WHERE tab_id = ? AND id < (SELECT tab_history_id FROM tab WHERE tab_id = ?) ORDER BY id DESC LIMIT 1;");
    query.bindValue(0, tabId);
    query.bindValue(1, tabId);
    if (!execute(query)) {
        return;
    }

    int historyId = 0;
    if (query.first()) {
        historyId = query.value(0).toInt();
    }

    if (historyId > 0) {
        if (updateTab(tabId, historyId)) {
            emit tabChanged(getTabData(tabId, historyId));
        }
    }
}

Link DBWorker::getCurrentLink(int tabId)
{
    int historyId = 0;
    QSqlQuery query = prepare("SELECT tab_history_id FROM tab WHERE tab_id = ?;");
    query.bindValue(0, tabId);
    if (execute(query)) {
        if (query.first()) {
            historyId = query.value(0).toInt();
        }
    } else {
        return Link();
    }
    return getLinkFromTabHistory(historyId);
}

Link DBWorker::getLinkFromTabHistory(int tabHistoryId)
{
    QSqlQuery query = prepare("SELECT link_id FROM tab_history WHERE id = ?;");
    query.bindValue(0, tabHistoryId);
    if (execute(query)) {
        if (query.first()) {
            return getLink(query.value(0).toInt());
        }
    }
    return Link();
}

int DBWorker::getPreviousLinkIdFromTabHistory(int tabHistoryId)
{
    QSqlQuery query = prepare("SELECT link_id FROM tab_history WHERE tab_id = (SELECT tab_id FROM tab_history WHERE id = ?) AND id < ? ORDER BY id DESC LIMIT 1;");
    query.bindValue(0, tabHistoryId);
    query.bindValue(1, tabHistoryId);
    if (execute(query)) {
        if (query.first()) {
            return query.value(0).toInt();
        }
    }
    return 0;
}

void DBWorker::clearDeprecatedTabHistory(int tabId, int currentLinkId) {
#if DEBUG_LOGS
    qDebug() << "tab id:" << tabId << "current link id:" << currentLinkId;
#endif
    QSqlQuery query = prepare("DELETE FROM tab_history WHERE tab_id = ? AND link_id > ?;");
    query.bindValue(0, tabId);
    query.bindValue(1, currentLinkId);
    execute(query);
}

int DBWorker::getNextLinkIdFromTabHistory(int tabHistoryId)
{
    QSqlQuery query = prepare("SELECT link_id FROM tab_history WHERE tab_id = (SELECT tab_id FROM tab_history WHERE id = ?) AND id > ? ORDER BY id ASC LIMIT 1;");
    query.bindValue(0, tabHistoryId);
    query.bindValue(1, tabHistoryId);
    if (execute(query)) {
        if (query.first()) {
            return query.value(0).toInt();
        }
    }
    return 0;
}

// Adds url to table history if it is not already there
HistoryResult DBWorker::addToBrowserHistory(QString url, QString title)
{
#if DEBUG_LOGS
    qDebug() << "link id:" << linkId;
#endif

    // Skip adding any urls with 'about:' prefix
    if (url.startsWith("about:")) {
        return Skipped;
    }
    QSqlQuery query = prepare("SELECT 1 FROM browser_history WHERE url = ?;");

    query.bindValue(0, url);
    if (!execute(query)) {
        return Error;
    }

    // Update history entry if it exists
    if (query.first()) {
        if (title.isEmpty()) {
            query = prepare("UPDATE browser_history SET date = ?, visited_count = visited_count + 1  WHERE url = ?;");
            query.bindValue(0, QDateTime::currentDateTimeUtc().toTime_t());
            query.bindValue(1, url);
        } else {
            query = prepare("UPDATE browser_history SET date = ?, title = ?, visited_count = visited_count + 1  WHERE url = ?;");
            query.bindValue(0, QDateTime::currentDateTimeUtc().toTime_t());
            query.bindValue(1, title);
            query.bindValue(2, url);
        }
        return execute(query) ? Added : Error;
    }

    // Otherwise create a new history entry
    query = prepare("INSERT INTO browser_history (url, title, date) VALUES (?, ?, ?);");
    query.bindValue(0, url);
    query.bindValue(1, title);
    query.bindValue(2, QDateTime::currentDateTimeUtc().toTime_t());
    return execute(query) ? Added : Error;
}

void DBWorker::clearHistory()
{
    int oldTabCount = tabCount();
    QSqlQuery query = prepare("DELETE FROM browser_history;");
    execute(query);
    removeAllTabs();
    query = prepare("DELETE FROM link;");
    execute(query);

    QList<Link> linkList;
    emit historyAvailable(linkList);
    if (oldTabCount != 0) {
        QList<Tab> tabList;
        emit tabsAvailable(tabList);
    }
}

int DBWorker::addToTabHistory(int tabId, int linkId)
{
    QSqlQuery query = prepare("INSERT INTO tab_history (tab_id, link_id, date) VALUES (?, ?, ?);");
    query.bindValue(0, tabId);
    query.bindValue(1, linkId);
    query.bindValue(2, QDateTime::currentDateTimeUtc().toTime_t());
    if (!execute(query)) {
        return 0;
    }

    QVariant lastId = query.lastInsertId();
    if (!lastId.isValid()) {
        qWarning() << Q_FUNC_INFO << "unable to fetch last inserted id";
        return 0;
    }

#if DEBUG_LOGS
    qDebug() << "tab:" << tabId << "link:" << linkId << "tab history id" << query.lastInsertId();
#endif
    return lastId.toInt();
}

void DBWorker::clearTabHistory(int tabId)
{
    // Remove urls that are only related to this tab
    QSqlQuery query = prepare("DELETE FROM link WHERE link_id IN "
                              "(SELECT DISTINCT link_id FROM tab_history WHERE tab_id = ? "
                              "AND link_id NOT IN (SELECT link_id FROM tab_history WHERE tab_id != ? "
                              "UNION SELECT link_id FROM tab_history WHERE id IN (SELECT tab_history_id FROM tab WHERE tab_id = ?)));");
    query.bindValue(0, tabId);
    query.bindValue(1, tabId);
    query.bindValue(2, tabId);
    execute(query);

    query = prepare("DELETE FROM tab_history WHERE tab_id = ? "
                    "AND id NOT IN (SELECT tab_history_id FROM tab WHERE tab_id = ?);");
    query.bindValue(0, tabId);
    query.bindValue(1, tabId);
    execute(query);

    emit tabChanged(getTabData(tabId));
}

int DBWorker::createLink(QString url, QString title, QString thumbPath)
{
    QSqlQuery query = prepare("INSERT INTO link (url, title, thumb_path) VALUES (?, ?, ?);");
    query.bindValue(0, url);
    query.bindValue(1, title);
    query.bindValue(2, thumbPath);
    execute(query);

    QVariant lastId = query.lastInsertId();
    if (!lastId.isValid()) {
        qWarning() << Q_FUNC_INFO << "unable to fetch last inserted id";
        return 0;
    }

#if DEBUG_LOGS
    qDebug() << title << url << thumbPath << lastId.toInt();
#endif
    int linkId = lastId.toInt();

    emit nextLinkId(linkId + 1);
    return linkId;
}

void DBWorker::getHistory(const QString &filter)
{
    // Skip empty titles always
    QString filterQuery("WHERE (NULLIF(title, '') IS NOT NULL AND url NOT LIKE 'about:%' AND %1) ");
    QString order;

    if (!filter.isEmpty()) {
        filterQuery = filterQuery.arg(QString("(url LIKE :search OR title LIKE :search)"));
        order = QString("LENGTH(url), title, date ASC");
    } else {
        filterQuery = filterQuery.arg(1);
        order = QString("date DESC");
    }

    QString queryString = QString("SELECT DISTINCT url, title "
                                  "FROM browser_history "
                                  "%1"
                                  "ORDER BY %2 LIMIT 20;").arg(filterQuery).arg(order);
    QSqlQuery query = prepare(queryString);
    if (!filter.isEmpty()) {
        query.bindValue(QString(":search"), QString("%%1%").arg(filter));
    }

    if (!execute(query)) {
        return;
    }

    QList<Link> linkList;
    while (query.next()) {
        Link url(0,
                 query.value(0).toString(),
                 "",
                 query.value(1).toString());
        linkList.append(url);
    }

    emit historyAvailable(linkList);
}

void DBWorker::getTabHistory(int tabId)
{
    QSqlQuery query = prepare("SELECT link.link_id, link.url, link.thumb_path, link.title "
                              "FROM tab_history "
                              "INNER JOIN link "
                              "ON tab_history.link_id=link.link_id "
                              "WHERE tab_history.tab_id = ? "
                              "ORDER BY tab_history.id DESC;");
    query.bindValue(0, tabId);
    if (!execute(query)) {
        return;
    }

    QList<Link> linkList;
    while (query.next()) {
        Link tmp(query.value(0).toInt(),
                query.value(1).toString(),
                query.value(2).toString(),
                query.value(3).toString());
        linkList.append(tmp);
    }

    emit tabHistoryAvailable(tabId, linkList);
}

void DBWorker::updateThumbPath(int tabId, QString path)
{
    m_updateThumbPathQuery.bindValue(0, path);
    m_updateThumbPathQuery.bindValue(1, tabId);
    if (execute(m_updateThumbPathQuery)) {
        emit thumbPathChanged(tabId, path);
    }
}

void DBWorker::updateTitle(int tabId, int linkId, QString url, QString title)
{
    Link link = getLink(linkId);
    QSqlQuery query = prepare("UPDATE link SET title = ? WHERE link_id = ?;");
    query.bindValue(0, title);
    query.bindValue(1, linkId);
    if (execute(query)) {
        if (link.isValid() && link.title() != title) {
            // For browsing history
            emit titleChanged(tabId, linkId, link.url(), title);
        }
    }

    query = prepare("UPDATE browser_history SET title = ? WHERE url = ?;");
    query.bindValue(0, title);
    query.bindValue(1, url);
    if (!execute(query)) {
        qWarning() << "Failed to add title to browser history";
    }
}

void DBWorker::saveSetting(QString name, QString value)
{
    QSqlQuery query = prepare("SELECT value FROM settings WHERE name = ?;");
    query.bindValue(0, name);
    if (!execute(query)) {
        return;
    }
    if (query.first()) {
        query = prepare("UPDATE settings SET value = ? WHERE name = ?;");
        query.bindValue(0, value);
        query.bindValue(1, name);
    } else {
        query = prepare("INSERT INTO settings (name, value) VALUES (?, ?);");
        query.bindValue(0, name);
        query.bindValue(1, value);
    }
    execute(query);
}

SettingsMap DBWorker::getSettings()
{
    QSqlQuery query = prepare("SELECT name,value FROM settings;");
    QMap<QString, QString> settings;
    if (execute(query)) {
        while (query.next()) {
            settings.insert(query.value(0).toString(), query.value(1).toString());
        }
    }
    return settings;
}

void DBWorker::deleteSetting(QString name)
{
    QSqlQuery query = prepare("DELETE FROM settings WHERE name = ?");
    query.bindValue(0, name);
    execute(query);
}


Link DBWorker::getLink(int linkId)
{
    QSqlQuery query = prepare("SELECT link_id, url, thumb_path, title FROM link WHERE link_id = ?;");
    query.bindValue(0, linkId);
    if (execute(query)) {
        if (query.first()) {
            return Link(query.value(0).toInt(),
                       query.value(1).toString(),
                       query.value(2).toString(),
                       query.value(3).toString());
        }
    }
    return Link();
}

Link DBWorker::getLink(QString url)
{
    if (url.isEmpty()) {
        return Link();
    }

    QSqlQuery query = prepare("SELECT link_id, url, thumb_path, title FROM link WHERE url = ?;");
    query.bindValue(0, url);
    if (execute(query)) {
        if (query.first()) {
            return Link(query.value(0).toInt(),
                       query.value(1).toString(),
                       query.value(2).toString(),
                       query.value(3).toString());
        }
    }
    return Link();
}

void DBWorker::updateLink(int linkId, QString url, QString title, QString thumbPath)
{
    // todo: check if an url in the db already contains url, then replace url
    QString queryBase = "UPDATE link SET ";
    int index = 0;
    int urlIndex = -1;
    int titleIndex = -1;
    int thumbIndex = -1;
    if (!url.isEmpty()) {
        queryBase.append("url = ?");
        urlIndex = index;
        index++;
    }
    if (!title.isEmpty()) {
        if (index > 0) {
            queryBase.append(", ");
        }
        queryBase.append("title = ?");
        titleIndex = index;
        index++;
    }
    if (!thumbPath.isEmpty()) {
        if (index > 0) {
            queryBase.append(", ");
        }
        queryBase.append("thumb_path = ?");
        thumbIndex = index;
        index++;
    }
    queryBase.append(" WHERE link_id = ?;");

    if (index == 0) {
        qWarning() << Q_FUNC_INFO << "empty paramters, doing nothing";
        return;
    }
    QSqlQuery query = prepare(queryBase.toUtf8().constData());
    if (urlIndex > -1) {
        query.bindValue(urlIndex, url);
    }
    if (titleIndex > -1) {
        query.bindValue(titleIndex, title);
    }
    if (thumbIndex > -1) {
        query.bindValue(thumbIndex, thumbPath);
    }
    query.bindValue(index, linkId);
    execute(query);
}
