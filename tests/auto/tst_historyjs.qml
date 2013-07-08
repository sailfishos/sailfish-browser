/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuickTest 1.0
import QtQuick 2.0
import "/usr/share/sailfish-browser/pages/history.js" as History

Item {
    WorkerScript {
        id: dbWorker
        source: "/usr/share/sailfish-browser/pages/dbWorker.js"
    }


    width: 100; height: 100

    resources: TestCase {
        name: "historyjs"
        when: windowShown

        property ListModel model: ListModel {}
        property ListModel tabs: ListModel {}

        property int initialCount

        function initTestCase() {
            History.loadHistory(model)
            History.loadTabs(tabs)
            History.loadSetting("test")
        }

        function test_addUrl() {
            var url = "http://test.com/sailfishos-browser-unit-test"
            var title = "test-1"
            var icon = {"source":"test-2"}
            var tabId = 13213
            History.addUrl(url, title, icon, tabId)
            History.loadTabHistory(tabId, model)

            var ok = false
            for (var i=0; i< model.count; i++) {
                var item = model.get(i)
                if (item.url === url && item.title === title && item.icon.path === icon.source ) {
                    ok = true
                    break
                }
            }
            compare(ok, true)

            // cleanup
            var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
            db.transaction(
                        function(tx) {
                            var result = tx.executeSql('DELETE FROM historytable WHERE url=?;',[url])
                            if (result.rowsAffected < 1) {
                                console.log("History remove failed")
                            }
                        });
            model.clear()
        }

        function test_deleteUrl() {
            var url = "http://test.com/sailfishos-browser-unit-test-2"
            var title = "test-1"
            var icon = {"source":"test-2"}
            var tabId = 13003

            History.addUrl(url, title, icon, tabId)
            History.deleteUrl(url)
            History.loadHistory(model)
            var ok = true
            for (var i=0; i< model.count; i++) {
                var item = model.get(i)
                if (item.url === url) {
                    ok = false
                    break
                }
            }
            compare(ok, true)
            model.clear()
        }

        function test_deleteTabHistory() {
            var tabId = 1001
            var url = "http://test.com/sailfishos-browser-unit-test-2"
            var title = "test-1"
            var icon = {"source":"test-2"}

            History.addUrl(url, title, icon, tabId)
            History.addUrl(url + "1", title, icon, tabId)
            History.addUrl(url + "2", title, icon, tabId)

            // asynch
            History.deleteTabHistory(tabId)
            wait(1000)

            History.loadTabHistory(tabId, model)
            compare(model.count, 0)
            model.clear()
        }

        function test_addTab() {
            var url = "http://test.com/sailfishos-browser-unit-test-4"
            var thumb ={"source":"/test/path1"}
            var tabId = History.addTab(url, thumb)
            History.loadTabs(tabs)

            var ok = false
            for (var i=0; i< tabs.count; i++) {
                var item = tabs.get(i)
                if (item.url === url && item.tabId === tabId && item.thumbPath.path === thumb.source) {
                    ok = true
                    break
                }
            }
            compare(ok, true)

            // cleanup
            var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
            db.transaction(
                        function(tx) {
                            var result = tx.executeSql('DELETE FROM tabs WHERE tab_id=?;',[tabId])
                            if (result.rowsAffected < 1) {
                                console.log("tabs remove failed")
                            }
                        });
            tabs.clear()
        }

        function test_deleteTab() {
            var url = "http://test.com/sailfishos-browser-unit-test-21"

            var tabId = History.addTab(url, {"source":""})
            History.deleteTab(tabId,"/path/to/file")

            // deleteTab is asynch
            wait(1000)

            History.loadTabs(tabs)

            var ok = true
            for (var i=0; i< tabs.count; i++) {
                var item = tabs.get(i)
                if (item.tabId === tabId) {
                    ok = false
                    break
                }
            }
            compare(ok, true)
            tabs.clear()
        }


        function test_updateTab() {
            var url = "http://test.com/sailfishos-browser-unit-test-10"
            var newUrl = "http://test.com/sailfishos-browser-unit-test-20"

            var tabId = History.addTab(url, {"source":""})

            //updateTab is async
            History.updateTab(tabId, newUrl,{"source":""})

            wait(1000)
            History.loadTabs(tabs)

            var ok = false
            for (var i=0; i< tabs.count; i++) {
                var item = tabs.get(i)
                if (item.url === newUrl && item.tabId === tabId) {
                    ok = true
                    break
                }
            }
            compare(ok, true)

            // cleanup
            var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
            db.transaction(
                        function(tx) {
                            var result = tx.executeSql('DELETE FROM tabs WHERE tab_id=?;',[tabId])
                            if (result.rowsAffected < 1) {
                                console.log("tabs remove failed")
                            }
                        });
            tabs.clear()
        }

        function test_deleteAllTabs() {
            var url = "http://test.com/sailfishos-browser-unit-test-21"

            History.addTab(url, {"source":""})
            History.addTab("Http://www.second.url",{"source":""})
            History.deleteAllTabs()
            History.loadTabs(tabs)

            compare(0, tabs.count)
            tabs.clear()
        }

        function test_loadSetting() {
            var sample = History.loadSetting("Should_not_exist")
            compare("",sample)

            var value = "http://www.sailfishos.org"
            var key = "test_key_for_parameters"
            // asynch
            History.saveSetting(key,value)
            wait(1000)

            var loaded = History.loadSetting(key)
            compare(value === loaded, true)

            var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
            db.transaction(
                        function(tx) {
                            var result = tx.executeSql('DELETE FROM settingtable WHERE setting_name=?',[key])
                        });
        }


        function test_saveSetting() {
            var value = "http://www.sailfishos.org_1"
            var value2 = "http://www.sailfishos.org_2"

            var key = "test_key_for_parameters_2"
            History.saveSetting(key,value)
            wait(1000)

            History.saveSetting(key, value2)
            wait(1000)

            var loaded = History.loadSetting(key)
            compare(value2, loaded)

            var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
            db.transaction(
                        function(tx) {
                            var result = tx.executeSql('DELETE FROM settingtable WHERE setting_name=?',[key])
                        });

        }


        function cleanupTestCase() {
            var db = openDatabaseSync("sailfish-browser","0.1","historydb", 100000)
            db.transaction(
                        function(tx) {
                            var result = tx.executeSql('DROP TABLE historytable')
                        });
            db.transaction(
                        function(tx) {
                            var result = tx.executeSql('DROP TABLE tabs')
                        });
            db.transaction(
                        function(tx) {
                            var result = tx.executeSql('DROP TABLE settingtable')
                        });
        }
    }
}
