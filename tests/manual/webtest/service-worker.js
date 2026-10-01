/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

"use strict";

self.addEventListener("install", function () {
  self.skipWaiting();
});

self.addEventListener("activate", function (event) {
  event.waitUntil(self.clients.claim());
});

self.addEventListener("message", function (event) {
  if (event.data && event.data.type === "ping" && event.ports && event.ports[0]) {
    event.ports[0].postMessage({ type: "pong", worker: "same-origin", at: new Date().toISOString() });
  }
});
