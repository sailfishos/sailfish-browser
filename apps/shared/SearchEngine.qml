import QtQuick 2.0
import Sailfish.Browser 1.0

DataFetcher {
    type: DataFetcher.OpenSearch

    property bool available
    property string url
    property string href
    property string title
    signal engineSaved

    function addSearchEngine() {
        fetch(href);
    }

    onStatusChanged: {
        if (status == DataFetcher.Ready) {
            engineSaved();
        }
    }
}
