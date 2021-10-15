/*
 * Copyright (c) 2020 Open Mobile Platform LLC.
 *
 * License: Proprietary
 */

Services.scriptloader.loadSubScript("chrome://embedlite/content/ClickEventBlocker.js", this);

let global = this;

// This will send "embed:OpenLink" message when a link is clicked.
ClickEventBlocker.init(global, {allowNavigationInSameOrigin: true});
