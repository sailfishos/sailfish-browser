var model = [].concat(
            [{
                 key: "",
                 //: Identifier of the current browser
                 //% "Gecko-based browser (Default)"
                 name: qsTrId("sailfish_browser-la-browser_list_current_browser")
             }],
            UserAgentManager.getBrowserList(),
            [{
                 custom: true,
                 //: A user-defined user agent
                 //% "Custom"
                 name: qsTrId("sailfish_browser-la-browser_list_custom")
             }]
            )

function getUserAgentString(userAgent, isKey)
{
    if (!isKey) {
        return userAgent
    }

    for (var i = 0; i < model.length; i++) {
        if (i in model && model[i].key === userAgent) {
            return model[i].name
        }
    }

    return model[0].name
}
