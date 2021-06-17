import QtQuick 2.0
import Sailfish.Silica 1.0

BookmarkEditDialog {
    property string icon
    property Component desktopBookmarkWriter
    property QtObject bookmarkWriterParent

    //% "Add to App Grid"
    description: qsTrId("sailfish_browser-he-add_bookmark_to_launcher")
    onAccepted: {
        if (desktopBookmarkWriter) {
            var bookmarkWriter = desktopBookmarkWriter.createObject(bookmarkWriterParent)
            bookmarkWriter.save(editedUrl, editedTitle, icon)
        } else {
            console.log("Cannot save to launcher without bookmark writter!!")
        }
    }
}
