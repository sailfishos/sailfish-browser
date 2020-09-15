/*
 * Copyright (c) 2020 Open Mobile Platform LLC.
 *
 * License: Proprietary
 */

Components.utils.import("resource://gre/modules/Services.jsm");

Services.scriptloader.loadSubScript("chrome://embedlite/content/ClickEventBlocker.js");

let global = this;

// This will send "embed:OpenLink" message when a link is clicked.
ClickEventBlocker.init(global, {allowNavigationInSameOrigin: true});
