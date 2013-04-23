function getDb() {
    return openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
}


function loadHistory(model) {
    var db = getDb()

    db.transaction(
                function(tx) {
                    tx.executeSql('CREATE TABLE IF NOT EXISTS history (url TEXT, title TEXT, icon TEXT)')
                });

    db.transaction(
                function(tx) {
                    var result = tx.executeSql('SELECT url, title, icon FROM history')

                    for (var i=0; i < result.rows.length; i++) {
                        model.insert(0, {"url": result.rows.item(i).url,
                                         "title": result.rows.item(i).title,
                                         "icon": result.rows.item(i).icon})
                    }
                });
    return db
}

function addUrl(url, title, icon) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT INTO history VALUES (?,?,?);',[url,title,icon])
                    if (result.rowsAffected < 1) {
                        console.log("History insert failed")
                    }
                });
}

function deleteUrl(url) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('DELETE FROM history WHERE url=?;',[url])
                    if (result.rowsAffected < 1) {
                        console.log("History remove failed")
                    }
                });
}

function loadTabs(model) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    tx.executeSql('CREATE TABLE IF NOT EXISTS tabs (tabId integer PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE, url TEXT)')
                });

    db.transaction(
                function(tx) {
                    var result = tx.executeSql('SELECT tabId, url FROM tabs')

                    for (var i=0; i < result.rows.length; i++) {
                        model.insert(0, {"tabId": result.rows.item(i).tabId,
                                         "url": result.rows.item(i).url})
                    }
                });
    return db
}

function addTab(url) {
    var db = getDb()
    var id
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT INTO tabs (url) VALUES (?);',[url])
                    if (result.rowsAffected < 1) {
                        console.log("Tab insert failed")
                    } else {
                        id = result.insertId
                    }
                });
    return id
}

function updateTab(tabId, url) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('UPDATE tabs SET url=? WHERE tabId=?;',[url, tabId])
                    if (result.rowsAffected < 1) {
                        console.log("Tab update failed")
                    }
                });

}

function deleteTab(tabId) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('DELETE FROM tabs WHERE tabId=?;',[tabId])
                    if (result.rowsAffected < 1) {
                        console.log("Tabs remove failed")
                    }
                });
}
