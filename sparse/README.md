# Sparse browser integration

The main Browser build produces `sailfish-browser`, `sailfish-browser-common`
and `sailfish-captiveportal`. Full Browser requires both shared runtime and
portal at the same version and release. The portal can also be installed alone.
Its network sign-in service and private browsing behavior remain available.
Common owns shared QML at both existing application data paths, preserving their
directory layout during upgrades; each application keeps its own top-level QML.

Build the separate `rpm/sparse/sailfish-browser-sparse.spec` from the same source
revision and version. This small package installs URL-handler desktop and D-Bus
service entries that run `sailfish-captiveportal -sparse`. It reuses the Browser
desktop identifier with `NoDisplay=true`, without providing the full Browser
package. It conflicts with full Browser and the public WebView RPM.

The WebView build separates its public `Sailfish.WebView` module into the
existing `sailfish-components-webview-qt5` package. Its common package contains
the engine library, `Sailfish.WebEngine` and `Sailfish.WebView.Controls`.
Popups and Pickers require this common package, so they can be installed without
the public module. Public TextZoomController remains a compatible wrapper.

## Build and install

Build the coordinated Lipstick, Jolla Home, managed Gecko patch stack, WebView
and Browser branches together. Lipstick installs the shared `intenturl.h`
header; Browser and Home must build against that updated development package.
Use the project SDK snapshot `browser-esr153` for every participating package.
Install coherent WebView runtime and development packages in that snapshot
before compiling Browser. QtMozEmbed needs no Sparse source changes.

In an SDK build shell prepared for the managed project snapshot, select the
Sparse metadata spec explicitly after the normal Browser build:

```sh
mb2 -t aarch64-browser-esr153 --no-snapshot=force --no-vcs-apply \
    --no-fix-version -s rpm/sparse/sailfish-browser-sparse.spec build --prepare
```

For full Browser, install full Browser, common runtime and portal, plus the
ordinary public WebView package and its support packages. For Sparse, remove
full Browser and public WebView in the same package-manager transaction that
installs Sparse. Keep WebView common, Popups, Pickers, QtMozEmbed, Browser common
and portal. Sparse intentionally does not satisfy dependencies on full Browser;
resolve any product package-pattern dependencies explicitly. Product image and
package-pattern changes are outside this implementation.

Switching back requires removing Sparse and installing full Browser and public
WebView. Do not remove shared runtimes while either application still uses them.
Restart Browser and its dedicated booster after replacing runtime files. Do not
mix a new Browser library with old Browser/portal executables. Inspect RPM
ownership, version requirements and both conflict installation orders before
changing an image or device. The standalone portal must coexist with full
Browser and public WebView.

## Runtime behavior

Sparse accepts HTTP/HTTPS URL launches through the Browser desktop identifier,
`org.sailfishos.browser` and `org.sailfishos.browser.ui`. Owned requestTab and
closeTab remain available. Empty launches and browsing-only service actions do
not open a homepage or tab picker. The normal `org.sailfishos.captiveportal`
service continues to run a separate network-login process.

Sparse uses application name `browser-sparse` under `org.sailfishos`, with a
persistent, separate profile. Network login stays private. Sparse tab bookkeeping
is transient: pages are not restored after restart, while cookies and saved
passwords persist. The toolbar provides Back, Reload/Stop, a read-only site,
Close and a confirmed Clear sign-in data action. Clearing affects only the
Sparse profile. Login dialogs, permissions, selection and downloads reuse the
existing engine/UI components. Transfer Cancel/Retry callbacks use the owning
process's service, and active transfers keep it alive after its window closes.

Non-web URLs are queried asynchronously through Lipstick with the complete URL.
Full Browser prompts before dispatch unless its existing site permission allows
it. Intent links always prompt and do not offer Remember, since their outer
scheme does not identify a single target application. Sparse dispatches supported links automatically, then closes the originating
root and its popup descendants while retaining independent flows. Dispatch means
Lipstick accepted the D-Bus call; it does not prove app launch or OAuth success.
Failed queries and unsupported links keep the flow open. HTTP/HTTPS navigation
stays internal.

Intent links accept only a validated target scheme, optional package,
VIEW action, BROWSABLE category and HTTP/HTTPS browser fallback. Components,
selectors, arbitrary actions/extras/flags, malformed encoding and native-app
wrapper packages are rejected. Query and dispatch share Lipstick's parser and
use the existing AppSupport APIs; no Android hosted-tab service or browser bridge
is part of this round. Package-constrained intents cannot select native handlers.
Full Browser prompt rejection or an unavailable handler follows a valid fallback
in the original security context. Navigation, closure or expiry cancels a stale
request without fallback. Identical fallback intents are suppressed within the
same fallback navigation chain to prevent redirect loops. A valid user gesture
or an independent document navigation allows a new attempt.

## Validation

Host tests cover the shared intent parser, asynchronous fileservice queries,
complete URL preservation, duplicate/cancelled requests, missing services,
fallback safety, popup-root closure and stale navigation. Run Browser's
`tst_externalurl` in an isolated session bus; `LIPSTICK_INCLUDE_DIR` can select
the development header. The hosted-QML `externalFlows` test exercises closure
and prompt-rejection routing. Gecko packaging supplies
`tests/test-external-chooser.cjs` for its managed patched source.

Host Qt is insufficient to confirm Qt 5.6, actual RPM ownership or native
presentation. Complete target builds and inspect installation, upgrade, removal,
conflicts in both orders, and standalone portal coexistence. Then exercise both
profiles together on a device: native rendering/input/rotation/selection,
password persistence/update/clear, dialogs and popup cancellation, concurrent
flows, downloads and transfer callbacks while UI closes. Verify native and
Android callbacks with full host/path/query/fragment matching, package constraints,
AppSupport unavailable, full Browser accept/decline/dismiss, Sparse automatic
handoff, fallback and no-handler cases. Device and image changes require a
separate deployment task; no deployment has been performed here.
