function loadModel(model) {

    var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)

    db.transaction(
                function(tx) {
                    tx.executeSql('CREATE TABLE IF NOT EXISTS history (url TEXT, title TEXT, icon TEXT)')
                });

    console.log("db open")
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('SELECT url, title, icon FROM history')

                    console.log("Result got")
                    for (var i=0; i < result.rows.length; i++) {
                        console.log("got " + result.rows.item(i).url + result.rows.item(i).title + result.rows.item(i).icon)



                        model.insert(0, {"url": result.rows.item(i).url,
                                         "title": result.rows.item(i).title,
                                         "icon": result.rows.item(i).icon})
                    }
                });

    console.log("done")

    return db
}


function addRow(url, title, icon) {
    var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
    console.log("saving" + url + title + icon)
    db.transaction(
                function(tx) {
                    var result = tx.executeSql('INSERT INTO history VALUES (?,?,?);',[url,title,icon])
                    if (result.rowsAffected < 1) {
                        console.log("History insert failed")
                    }
                });
}
