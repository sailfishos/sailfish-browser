import QtQuick 1.1
import Sailfish.Silica 1.0
import org.sailfishos.browser.settings 1.0

Page {
    Label {
        anchors.centerIn: parent
        //% "Hello world!"
        text: qsTrId("hello_world") + "<br>" + settings.testProp
    }

    BrowserSettings {
        id: settings
    }
}
