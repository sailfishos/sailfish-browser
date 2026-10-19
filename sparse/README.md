# Sparse browser

Sparse provides a small browser window for links and sign-in pages, using the
captive portal application. It has no launcher icon, homepage or tab picker.
It handles HTTP(S) URLs through the Browser desktop entry and the
`org.sailfishos.browser` and `org.sailfishos.browser.ui` D-Bus services.

The `sailfish-browser-sparse` package replaces full Browser and the public
WebView package. It uses Browser common, the captive portal, WebView core,
Popups and Pickers. Ordinary network sign-in and
[caller-owned portal flows](../apps/captiveportal/flows/README.md) run separately.

## Browsing and sign-in data

Sparse keeps its own persistent profile. Cookies and saved passwords survive
restarts, but open pages do not. Network sign-in uses a separate private session.

The toolbar provides Back, Reload/Stop, the site address, Close and Clear sign-in
data. Clearing requires confirmation and affects only Sparse's cookies, site
data and saved passwords. Active downloads can continue after the window closes.

Web links open inside Sparse. Supported external links open in their registered
application, then close the originating page and its popups. Other open pages
remain. Unsupported links leave the page open; supported Android intent links
can use an HTTP(S) fallback when their target application is unavailable.

## Cross-site links

Sparse allows cross-site HTTP(S) links by default. To block links to a different
origin, run this as the device session user, then fully quit and restart Sparse:

```sh
dconf write /apps/sailfish-browser/settings/sparse_block_cross_site_links true
```

To restore the default:

```sh
dconf reset /apps/sailfish-browser/settings/sparse_block_cross_site_links
```

The first top-level HTTP(S) page establishes the allowed origin; frames cannot
change it. When blocking is enabled, a link to a different scheme, host or port
leaves the current page open and shows a notice. Forms, redirects, script-driven
navigation and external-app links are unaffected. This setting applies only to Sparse;
ordinary network sign-in and caller-owned authentication flows keep their own
navigation behavior.
