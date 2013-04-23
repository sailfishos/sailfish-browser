/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuickTest 1.0
import QtQuick 1.1
import "/usr/share/sailfish-browser/pages/history.js" as History

Item {
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
        }

        function test_addUrl() {
            var url = "http://test.com/sailfishos-browser-unit-test"
            var title = "test-1"
            var icon = "test-2"
            var tabId = 13213
            History.addUrl(url, title, icon, tabId)
            History.loadTabHistory(tabId, model)

            var ok = false
            for (var i=0; i< model.count; i++) {
                var item = model.get(i)
                if (item.url === url && item.title === title && item.icon === icon ) {
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
            var icon = "test-2"
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

        function test_addTab() {
            var url = "http://test.com/sailfishos-browser-unit-test-4"
            var thumb ="/test/path1"
            var tabId = History.addTab(url, thumb)
            History.loadTabs(tabs)

            var ok = false
            for (var i=0; i< tabs.count; i++) {
                var item = tabs.get(i)
                if (item.url === url && item.tabId === tabId && item.thumbPath === thumb) {
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

            var tabId = History.addTab(url)
            History.deleteTab(tabId,"/path/to/file")
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

            var tabId = History.addTab(tabId, url)
            History.updateTab(tabId, newUrl)
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

            History.addTab(url, "thumbpath")
            History.addTab("Http://www.second.url")
            History.deleteAllTabs()
            History.loadTabs(tabs)

            compare(0, tabs.count)
            tabs.clear()
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
        }
    }
}
