/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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

#include "dbworker.h"
#include "browserpaths.h"
#include "faviconmanager.h"

#ifndef DEBUG_LOGS
#define DEBUG_LOGS 0
#endif

#define DB_USER_VERSION 1

#define QUOTE(arg) #arg
#define STR(arg) QUOTE(arg)

#define MAX_BROWSER_HISTORY_SIZE 2000

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

static const char * const create_table_user_agent_overrides =
        "CREATE TABLE user_agent_overrides (\n"
        "   host TEXT NOT NULL UNIQUE PRIMARY KEY,\n"
        "   is_key INTEGER NOT NULL CHECK(is_key IN (0,1)),\n"
        "   user_agent TEXT\n"
        ") WITHOUT ROWID\n";

static const char * const set_user_version =
        "PRAGMA user_version=" STR(DB_USER_VERSION) ";\n";

static const char *db_schema[] = {
    create_table_tab,
    create_table_tab_history,
    create_table_link,
    create_table_browser_history,
    create_table_settings,
    create_table_user_agent_overrides,
    set_user_version
};
static int db_schema_count = sizeof(db_schema) / sizeof(*db_schema);

DBWorker::DBWorker(QObject *parent) :
    QObject(parent)
{
}

void DBWorker::init()
{
    QString databaseDir = BrowserPaths::dataLocation();
    if (databaseDir.isNull()) {
        return;
    }
    QDir dir(databaseDir);
    const QString dbFileName(QLatin1String(DB_NAME));

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
    } else {
        // Limit history size to 2000 entries
        QSqlQuery cleanupHistory = prepare("DELETE FROM browser_history WHERE id NOT IN (SELECT id from browser_history"\
                                           " ORDER BY date DESC LIMIT " STR(MAX_BROWSER_HISTORY_SIZE) ");");
        if (!execute(cleanupHistory)) {
            qWarning() << "Failed to clear older history items";
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

void DBWorker::createTab(const Tab &tab)
{
#if DEBUG_LOGS
    qDebug() << "new tab id: " << tab.tabId();
#endif
    QSqlQuery query = prepare("INSERT INTO tab (tab_id, tab_history_id) VALUES (?,?);");
    query.bindValue(0, tab.tabId());
    query.bindValue(1, 0);
    execute(query);

    if (tab.url().isEmpty()) {
        return;
    }

    int linkId = createLink(tab.url(), tab.title(), tab.thumbnailPath());

    int historyId = addToTabHistory(tab.tabId(), linkId);
    addHistoryEntry(tab.url(), tab.title());
    if (historyId > 0) {
        updateTab(tab.tabId(), historyId);
    } else {
        qWarning() << Q_FUNC_INFO << "failed to add url to tab history" << tab.url();
    }

#if DEBUG_LOGS
    qDebug() << "created link:" << linkId << "with history id:" << historyId << "for tab:" << tab.tabId() << tab.url();
#endif
}

void DBWorker::updateTab(int tabId, int tabHistoryId)
{
#if DEBUG_LOGS
    qDebug() << "tab:" << tabId << "tab history id:" << tabHistoryId;
#endif
    QSqlQuery query = prepare("UPDATE tab SET tab_history_id = ? WHERE tab_id = ?;");
    query.bindValue(0, tabHistoryId);
    query.bindValue(1, tabId);
    execute(query);
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

void DBWorker::removeAllTabs(bool noFeedback)
{
    int oldTabCount(0);

    if (!noFeedback) {
        oldTabCount = tabCount();
    }

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

void DBWorker::getAllTabs()
{
    QList<Tab> tabList;
    QSqlQuery query = prepare("SELECT tab.tab_id, link.url, link.title, link.thumb_path "
                              "FROM tab "
                              "INNER JOIN tab_history ON tab_history.id = tab.tab_history_id "
                              "INNER JOIN link ON tab_history.link_id = link.link_id;");
    if (!execute(query)) {
        return;
    }

    while (query.next()) {
        tabList.append(Tab(query.value(0).toInt(),
                           query.value(1).toString(),
                           query.value(2).toString(),
                           query.value(3).toString()));
    }
    emit tabsAvailable(tabList);
}

int DBWorker::getMaxTabId()
{
    return integerQuery("SELECT MAX(tab_id) FROM tab;");
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

void DBWorker::navigateTo(int tabId, const QString &url, const QString &title, const QString &path) {
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

    int historyId = addToTabHistory(tabId, linkId);
    addHistoryEntry(url, title);
    if (historyId > 0) {
        updateTab(tabId, historyId);
    } else {
        qWarning() << Q_FUNC_INFO << "failed to add url to tab history" << url;
    }

#if DEBUG_LOGS
    qDebug() << "emit tab changed:" << tabId << historyId << title << url;
#endif
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
        updateTab(tabId, historyId);
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
        updateTab(tabId, historyId);
    }
}

Link DBWorker::getCurrentLink(int tabId)
{
    QSqlQuery query = prepare("SELECT link.link_id, link.url, link.thumb_path, link.title "
                              "FROM tab "
                              "INNER JOIN tab_history ON tab_history.id = tab.tab_history_id "
                              "INNER JOIN link ON tab_history.link_id = link.link_id "
                              "WHERE tab.tab_id = ?;");
    query.bindValue(0, tabId);
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

void DBWorker::clearDeprecatedTabHistory(int tabId, int currentLinkId) {
#if DEBUG_LOGS
    qDebug() << "tab id:" << tabId << "current link id:" << currentLinkId;
#endif
    QSqlQuery query = prepare("DELETE FROM tab_history WHERE tab_id = ? AND link_id > ?;");
    query.bindValue(0, tabId);
    query.bindValue(1, currentLinkId);
    execute(query);
}

void DBWorker::addHistoryEntry(const QString &url, const QString &title)
{
#if DEBUG_LOGS
    qDebug() << "url:" << url << "title:" << title;
#endif

    // Skip adding any urls with 'about:' prefix
    if (url.startsWith("about:")) {
        return;
    }
    QSqlQuery query = prepare("SELECT 1 FROM browser_history WHERE url = ?;");

    query.bindValue(0, url);
    if (!execute(query)) {
        return;
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
        execute(query);
    } else {
        // Otherwise create a new history entry
        query = prepare("INSERT INTO browser_history (url, title, date) VALUES (?, ?, ?);");
        query.bindValue(0, url);
        query.bindValue(1, title);
        query.bindValue(2, QDateTime::currentDateTimeUtc().toTime_t());
        execute(query);
    }
}

void DBWorker::clearHistory(int period)
{
    QSqlQuery query;

    if (period == 0) { // Delete everything
        FaviconManager::instance()->clear(QStringLiteral("history"));
        query = prepare("DELETE FROM browser_history;");
    } else {
        QList<Link> historyList = getHistoryQList();
        QSet<QString> historyIconsToRemove;

        int historySize = historyList.size();
        int boundaryIndex = 0;

        for (int i = 0; i < historySize; i++) {
            const QString sanitizedUrl = FaviconManager::sanitizedHostname(historyList[i].url());
            if (historyList[i].date().daysTo(QDate::currentDate()) < period) {
                historyIconsToRemove.insert(sanitizedUrl);
            } else if (historyList[i].date().daysTo(QDate::currentDate()) >= period) {
                boundaryIndex = i;
                break;
            }
            boundaryIndex = i + 1;
        }

        for (int i = boundaryIndex; i < historySize; i++) {
            const QString sanitizedUrl = FaviconManager::sanitizedHostname(historyList[i].url());
            historyIconsToRemove.remove(sanitizedUrl);
        }

        QSet<QString>::iterator iconIterator;
        for (const QString iconToRemove : historyIconsToRemove) {
            FaviconManager::instance()->remove(QStringLiteral("history"), iconToRemove);
        }

        uint boundaryTimeT = QDateTime::currentDateTimeUtc().addDays(-period).toTime_t();

        query = prepare("DELETE FROM browser_history WHERE date > ?");
        query.bindValue(0, boundaryTimeT);
    }

    execute(query);
    removeAllTabs();
    query = prepare("DELETE FROM link;");
    execute(query);

    QList<Link> linkList;
    emit historyAvailable(linkList);
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

int DBWorker::createLink(const QString &url, const QString &title, const QString &thumbPath)
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
    return linkId;
}

void DBWorker::getHistory(const QString &filter)
{
    QString filterQuery("WHERE url NOT LIKE 'about:%' AND %1 ");
    QString order;

    if (!filter.isEmpty()) {
        filterQuery = filterQuery.arg(QString("(url LIKE :search OR title LIKE :search)"));
        order = QString("date DESC, visited_count DESC, LENGTH(url), title");
    } else {
        filterQuery = filterQuery.arg(1);
        order = QString("date DESC");
    }

    QString queryString = QString("SELECT id, url, title, date, visited_count "
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
        qint64 timestamp = query.value(3).toLongLong();
        Link link(query.value(0).toInt(),
                  query.value(1).toString(),
                  "",
                  query.value(2).toString(),
                  QDateTime::fromMSecsSinceEpoch(timestamp*1000).date());
#if DEBUG_LOGS
        qDebug() << &link << "visitedCount:" << query.value(4).toInt();
#endif
        linkList.append(link);
    }

    emit historyAvailable(linkList);
}

QList<Link> DBWorker::getHistoryQList()
{
    QString filterQuery("WHERE url NOT LIKE 'about:%'");
    QString order;

    order = QString("date DESC");

    QString queryString = QString("SELECT id, url, title, date, visited_count "
                                  "FROM browser_history "
                                  "%1"
                                  "ORDER BY %2;").arg(filterQuery).arg(order);
    QSqlQuery query = prepare(queryString);

    execute(query);

    QList<Link> linkList;
    while (query.next()) {
        qint64 timestamp = query.value(3).toLongLong();
        Link link(query.value(0).toInt(),
                  query.value(1).toString(),
                  "",
                  query.value(2).toString(),
                  QDateTime::fromMSecsSinceEpoch(timestamp*1000).date());

        linkList.append(link);
    }

    return linkList;
}

void DBWorker::getTabHistory(int tabId)
{
    QSqlQuery query = prepare("SELECT link.link_id, link.url, link.thumb_path, link.title, (tab_history.id == tab.tab_history_id) AS current "
                              "FROM tab_history "
                              "INNER JOIN tab ON tab.tab_id = tab_history.tab_id "
                              "INNER JOIN link ON tab_history.link_id = link.link_id "
                              "WHERE tab_history.tab_id = ? "
                              "ORDER BY tab_history.id DESC;");
    query.bindValue(0, tabId);
    if (!execute(query)) {
        return;
    }

    QList<Link> linkList;
    int currentLinkId(-1);
    while (query.next()) {
        int linkId = query.value(0).toInt();
        Link tmp(linkId,
                query.value(1).toString(),
                query.value(2).toString(),
                query.value(3).toString());
        linkList.append(tmp);
        if (query.value(4).toBool()) {
            currentLinkId = linkId;
        }
    }

    emit tabHistoryAvailable(tabId, linkList, currentLinkId);
}

void DBWorker::removeHistoryEntry(int linkId)
{
    QSqlQuery query = prepare("DELETE FROM browser_history WHERE id = ?");
    query.bindValue(0, linkId);
    execute(query);
}

void DBWorker::removeHistoryEntry(const QString &url)
{
    QSqlQuery query = prepare("DELETE FROM browser_history WHERE url = ?");
    query.bindValue(0, url);
    execute(query);
}

void DBWorker::updateThumbPath(int tabId, const QString &path)
{
    m_updateThumbPathQuery.bindValue(0, path);
    m_updateThumbPathQuery.bindValue(1, tabId);
    if (execute(m_updateThumbPathQuery)) {
        emit thumbPathChanged(tabId, path);
    }
}

void DBWorker::updateUrl(int tabId, const QString &requestedUrl, const QString &resolvedUrl)
{
    QSqlQuery query = prepare("SELECT link.link_id, link.url FROM tab "
                              "INNER JOIN tab_history ON tab.tab_history_id = tab_history.id "
                              "INNER JOIN link ON tab_history.link_id = link.link_id "
                              "WHERE tab_history.tab_id = ?;");
    query.bindValue(0, tabId);
    if (!execute(query)) {
        qWarning() << "No link found for tabId" << tabId;
        return;
    }

    if (query.first()) {
        int linkId = query.value(0).toInt();
        QString oldUrl = query.value(1).toString();

        if (linkId > 0 && !oldUrl.isEmpty() && oldUrl != resolvedUrl) {
            query = prepare("UPDATE link SET url = ? WHERE link_id = ?;");
            query.bindValue(0, resolvedUrl);
            query.bindValue(1, linkId);
            execute(query);
        }
    }

    if (!requestedUrl.isEmpty() && (requestedUrl != resolvedUrl)) {
        removeHistoryEntry(requestedUrl);
    }
    addHistoryEntry(resolvedUrl, QString());
}

void DBWorker::updateTitle(int tabId, const QString &url, const QString &title)
{
    // TODO: add DB indices
    QSqlQuery query = prepare("SELECT link.link_id, link.url, link.title FROM tab "
                              "INNER JOIN tab_history ON tab.tab_history_id = tab_history.id "
                              "INNER JOIN link ON tab_history.link_id = link.link_id "
                              "WHERE tab_history.tab_id = ?;");
    query.bindValue(0, tabId);
    if (!execute(query)) {
        qWarning() << "No link found for tabId" << tabId;
        return;
    }

    bool historyUpdated = false;

    if (query.first()) {
        int linkId = query.value(0).toInt();
        QString oldUrl = query.value(1).toString();
        QString oldTitle = query.value(2).toString();

        if (linkId > 0 && oldUrl.length() > 0 && oldTitle != title) {
            query = prepare("UPDATE link SET title = ? WHERE link_id = ?;");
            query.bindValue(0, title);
            query.bindValue(1, linkId);
            if (execute(query)) {
                historyUpdated = true;
            } else {
                qWarning() << "Failed to update link's title";
            }
        }
    }

    query = prepare("UPDATE browser_history SET title = ? WHERE url = ?;");
    query.bindValue(0, title);
    query.bindValue(1, url);
    if (execute(query)) {
        historyUpdated = true;
    } else {
        qWarning() << "Failed to add title to browser history";
    }

    if (historyUpdated) {
        // For browsing history
        emit titleChanged(url, title);
    }
}

void DBWorker::saveSetting(const QString &name, const QString &value)
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

void DBWorker::deleteSetting(const QString &name)
{
    QSqlQuery query = prepare("DELETE FROM settings WHERE name = ?");
    query.bindValue(0, name);
    execute(query);
}

void DBWorker::setUserAgentOverride(const QString &host, const bool isKey, const QString &userAgent)
{
    QSqlQuery query = prepare("INSERT INTO user_agent_overrides "
                              "(host, is_key, user_agent) "
                              "VALUES (?, ?, ?) "
                              "ON CONFLICT(host) DO UPDATE SET "
                              "is_key=excluded.is_key, "
                              "user_agent=excluded.user_agent;");

    query.bindValue(0, host);
    query.bindValue(1, isKey);
    query.bindValue(2, userAgent);
    execute(query);
}

void DBWorker::unsetUserAgentOverride(const QString &host)
{
    QSqlQuery query = prepare("DELETE FROM user_agent_overrides WHERE host = ?");
    query.bindValue(0, host);
    execute(query);
}

void DBWorker::clearUserAgentOverrides()
{
    QSqlQuery query = prepare("DELETE FROM user_agent_overrides");
    execute(query);
}

QVariantMap DBWorker::getUserAgentOverrides()
{
    QSqlQuery query = prepare("SELECT host, is_key, user_agent FROM user_agent_overrides;");
    QVariantMap userAgentOverrides;
    if (execute(query)) {
        while (query.next()) {
            userAgentOverrides.insert(query.value(0).toString(),
                                      QVariantList({query.value(1).toBool(),
                                                    query.value(2).toString()}));
        }
    }
    return userAgentOverrides;
}
