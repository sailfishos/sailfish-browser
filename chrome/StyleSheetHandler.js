/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

dump("### StyleSheetHandler.js loaded\n");

var StyleSheetHandler = {
    init: function() {
        addMessageListener("Browser:SelectionColorUpdate", this);
    },

    shutdown: function() {
        removeMessageListener("Browser:SelectionColorUpdate", this);
    },

    receiveMessage: function(aMessage) {
        let json = aMessage.json;
        content.document.styleSheets[0].insertRule("::-moz-selection {background: " + json.color + "; }", 0);
    }
};

StyleSheetHandler.init();