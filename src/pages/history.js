function getDb() {
    return LocalStorage.openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
}


function loadHistory(model) {
    var db = getDb()

    db.transaction(
                function(tx) {
                    tx.executeSql('CREATE TABLE IF NOT EXISTS historytable (url TEXT, title TEXT, icon TEXT, tab_id INTEGER)')
                });

    db.transaction(
                function(tx) {
                    var result = tx.executeSql('SELECT url, title, icon, tab_id FROM historytable')

                    for (var i=0; i < result.rows.length; i++) {
                        model.insert(0, {"url": result.rows.item(i).url,
                                         "title": result.rows.item(i).title,
                                         "icon": { "path" : result.rows.item(i).icon },
                                         "tabId":result.rows.item(i).tab_id})
                    }
                });
    return db
}

function loadSetting(setting_name) {
    var db = getDb()

    var retval =""

    db.transaction(
                function(tx) {
                    tx.executeSql('CREATE TABLE IF NOT EXISTS settingtable (setting_name TEXT UNIQUE, setting_value TEXT)')
                });

    db.transaction(
                function(tx) {
                    var result = tx.executeSql('SELECT setting_value, setting_name FROM settingtable WHERE setting_name=(?)',[setting_name])
                    if(result.rows.length > 0) {
                        retval = result.rows.item(0).setting_value;
                    }
                });
    return retval
}

function saveSetting(setting_name, value) {
    dbWorker.sendMessage({"operation":"saveSetting", "setting_name": setting_name, "value":value})
}



function loadTabHistory(tabId, model) {
    var db = getDb()

    db.transaction(
                function(tx) {
                    tx.executeSql('CREATE TABLE IF NOT EXISTS historytable (url TEXT, title TEXT, icon TEXT, tab_id INTEGER)')
                });

    db.transaction(
                function(tx) {
                    var result = tx.executeSql('SELECT url, title, icon FROM historytable WHERE tab_id=(?)',[tabId])

                    for (var i=0; i < result.rows.length; i++) {
                        model.insert(0, {"url": result.rows.item(i).url,
                                         "title": result.rows.item(i).title,
                                         "icon": {"path":result.rows.item(i).icon}})
                    }
                });
    return db
}

function addUrl(url, title, icon, tabId) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT INTO historytable VALUES (?,?,?,?);',[url,title,icon.source,tabId])
                    if (result.rowsAffected < 1) {
                        console.log("historytable insert failed")
                    }
                });
}

function deleteUrl(url) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('DELETE FROM historytable WHERE url=?;',[url])
                    if (result.rowsAffected < 1) {
                        console.log("historytable remove failed")
                    }
                });
}

function deleteTabHistory(tabId) {
    dbWorker.sendMessage({"operation":"deleteTabHistory","tabId":tabId})
}

function loadTabs(model) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    tx.executeSql('CREATE TABLE IF NOT EXISTS tabs (tab_id integer PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE, url TEXT, thumb_path TEXT)')
                });

    db.transaction(
                function(tx) {
                    var result = tx.executeSql('SELECT tab_id, url, thumb_path FROM tabs')

                    for (var i=0; i < result.rows.length; i++) {
                        model.insert(0, {"tabId": result.rows.item(i).tab_id,
                                         "url": result.rows.item(i).url,
                                         "thumbPath": {"path":result.rows.item(i).thumb_path }})
                    }
                });
    return db
}

function addTab(url, thumb) {
    var db = getDb()
    var id
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT INTO tabs (url, thumb_path) VALUES (?, ?);',[url, thumb.source])
                    if (result.rowsAffected < 1) {
                        console.log("Tab insert failed")
                    } else {
                        id = result.insertId
                    }
                });
    return id
}

function updateTab(tabId, url, thumb) {
    dbWorker.sendMessage({"operation":"updateTab", "tabId":tabId, "url": url, "thumb": thumb.source})
}

function deleteTab(tabId) {
    dbWorker.sendMessage({"operation":"deleteTab","tabId":tabId})
}

function deleteAllTabs() {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('DELETE FROM tabs')
                });
}
