function getDb() {
    return openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
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
                                         "icon": result.rows.item(i).icon,
                                         "tabId":result.rows.item(i).tab_id})
                    }
                });
    return db
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
                                         "icon": result.rows.item(i).icon})
                    }
                });
    return db
}

function addUrl(url, title, icon, tabId) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT INTO historytable VALUES (?,?,?,?);',[url,title,icon,tabId])
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
                                         "thumbPath": result.rows.item(i).thumb_path })
                    }
                });
    return db
}

function addTab(url, thumbPath) {
    var db = getDb()
    var id
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT INTO tabs (url, thumb_path) VALUES (?, ?);',[url, thumbPath])
                    if (result.rowsAffected < 1) {
                        console.log("Tab insert failed")
                    } else {
                        id = result.insertId
                    }
                });
    return id
}

function updateTab(tabId, url, thumbPath) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('UPDATE tabs SET url=?, thumb_path=? WHERE tab_id=?;',[url, thumbPath, tabId])
                    if (result.rowsAffected < 1) {
                        console.log("Tab update failed")
                    }
                });

}

function deleteTab(tabId) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('DELETE FROM tabs WHERE tab_id=?;',[tabId])
                    if (result.rowsAffected < 1) {
                        console.log("Tabs remove failed")
                    }
                });
}

function deleteAllTabs() {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('DELETE FROM tabs')
                });
}
