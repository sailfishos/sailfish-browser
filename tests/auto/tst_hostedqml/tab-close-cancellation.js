/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var normalModel = {};
var privateModel = {};
var tabId = 17;
var destroying = true;
var view = {model: {sourceModel: normalModel}};
var root = {width: 400, height: 500, contentItem: {x: -400}};

restoreRejectedClose("18", normalModel);
check(destroying, "Another tab's rejection restored this card");
restoreRejectedClose("17", privateModel);
check(destroying, "Private tab ID collision restored a normal card");
restoreRejectedClose("17", normalModel);
check(!destroying, "Rejected close left the card hidden and disabled");
equal(root.contentItem.x, 0, "Swiped card remained offscreen");
root.contentItem.x = -20;
restoreRejectedClose("17", normalModel);
equal(root.contentItem.x, -20, "Unrelated repeat rejection reset an active drag");
true;
