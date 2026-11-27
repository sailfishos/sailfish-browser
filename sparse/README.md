# Sparse browser

Sparse provides a small browser window for links and sign-in pages, using the
captive portal application. It has no launcher icon, homepage or tab picker.
It handles HTTP(S) URLs and local PDF documents through the Browser desktop entry and the
`org.sailfishos.browser` and `org.sailfishos.browser.ui` D-Bus services.

Local launches accept readable PDF files, including file URLs and filesystem
paths. Other local document types are not accepted. PDFs use native Find and
Download controls in the bottom toolbar. The sandbox permits user
directories and removable media so documents can be opened from Files.

The `sailfish-browser-sparse` package replaces full Browser and the public
WebView package. It uses Browser common, the captive portal, WebView core,
Popups and Pickers. Ordinary network sign-in and
[caller-owned portal flows](../apps/captiveportal/flows/README.md) run separately.

## Browsing and sign-in data

Sparse keeps its own persistent profile. Cookies and saved passwords survive
restarts, but open pages do not. Network sign-in uses a separate private session.

The toolbar provides Back, Reload/Stop, the site address and Close. On devices
with softkeys, the left key provides Back when history is available and Close
otherwise; the right key provides Stop while loading and Refresh otherwise.
The duplicate Back and Reload/Stop icons are hidden. Text input retains its
editing actions, and the middle key remains available for page interaction.
PDF pages also provide Find and Download. Active downloads can continue after
the window closes.

Web links open inside Sparse. Supported external links open in their registered
application, then close the originating page and its popups. Other open pages
remain. Unsupported links leave the page open; supported Android intent links
can use an HTTP(S) fallback when their target application is unavailable.

## Cross-site links

The first top-level HTTP(S) page in each Sparse tab establishes its allowed
origin. Clicking a web link to a different scheme, host or port leaves the
current page open and shows a notice. Frames cannot change the allowed origin.

Forms, redirects, script-driven navigation and external-app links keep their
existing behaviour. The full browser, ordinary network sign-in and caller-owned
authentication flows retain their own navigation policies.
