WorkerScript.onMessage = function(message) {

    switch(message.operation)
    {
    case "updateTab":
        updateTab(message.tabId, message.url, message.thumb)
        break
    case "deleteTab":
        deleteTab(message.tabId)
        break
    case "deleteTabHistory":
        deleteTabHistory(message.tabId)
        break
    case "saveSetting":
        saveSetting(message.setting_name, message.value)
        break

    default:
        console.log("Uknown message" + message)
    }

}

function getDb() {
    return openDatabaseSync("sailfish-browser", "0.1", "historydb", 100000)
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
