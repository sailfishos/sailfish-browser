WorkerScript.onMessage = function(message) {
    // TODO get rid of this file and rewrite historydb handling in native
    // so that we have asynch writes
    console.log("dbWorker message" + message)
}

function getDb() {
    return LocalStorage.openDatabaseSync("sailfish-browser", "0.1", "historydb", 100000)
}

function updateTab(tabId, url, thumb) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('UPDATE tabs SET url=?, thumb_path=? WHERE tab_id=?;',[url, thumb, tabId])
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

function deleteTabHistory(tabId) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('DELETE FROM historytable WHERE tab_id=?;',[tabId])
                });
}

function saveSetting(setting_name, value) {
    var db = getDb()
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT OR REPLACE INTO settingtable VALUES (?,?);',[setting_name,value])
                    if (result.rowsAffected < 1) {
                        console.log("parameters insert failed")
                    }
                });
}
