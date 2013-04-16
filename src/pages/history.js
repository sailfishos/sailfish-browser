function loadModel(model) {

    var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)

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


function addRow(url, title, icon) {
    var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT INTO history VALUES (?,?,?);',[url,title,icon])
                    if (result.rowsAffected < 1) {
                        console.log("History insert failed")
                    }
                });
}
