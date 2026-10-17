# Captive portal flows

The standalone portal can render account authentication and terms pages without
the public `Sailfish.WebView` module. Require `sailfish-captiveportal` and
`sailfish-captiveportal-flow-api >= 2`. These flows use the ordinary portal's
private session, separately from Sparse's persistent sign-in profile.

The session-bus service is `org.sailfishos.captiveportal`, object `/flows`,
interface `org.sailfishos.captiveportal.Flow1`. Invoke methods asynchronously:

| Method | Arguments |
| --- | --- |
| `openFlow` | request ID, HTTP(S) URL, purpose, callback URL |
| `openFlowWithHeading` | request ID, HTTP(S) URL, purpose, callback URL, heading |
| `closeFlow` | request ID |
| `waitForEvent` | request ID; returns an event dictionary |

Purpose is `terms` or `authentication`. The caller chooses a request ID before
opening. IDs belong to the caller's unique bus name; another connection cannot
close them or collect their results. After opening, call `waitForEvent` and re-arm
it after each reply. Only one wait may be outstanding per flow. Results are
returned as method replies, which work through Sailjail without granting access
to arbitrary caller names. OAuth URLs are never broadcast.

| Dictionary `event` | Other fields |
| --- | --- |
| `updated` | `url`, `title` |
| `loaded` | `secure` (TLS security state) |
| `failed` | `reason`: `load`, `tls` or `overflow` |
| `closed` | `reason`: `completed`, `cancelled` or `failed` |

An idle wait returns an empty dictionary after 20 seconds; re-arm it normally.
Events between waits are buffered in order, including intermediate callback URLs.
The buffer allows 64 events and 1 MiB of URL/title text per flow. Overflow fails
and closes the flow instead of silently discarding an authentication result.
After closure, uncollected results expire after five seconds. Portal shutdown
waits for these final results to be collected or expire; disconnected owners
release their results immediately.

For authentication, preserve the registered provider callback URL. Scheme,
host, port, path and fixed query parameters must match before a custom-protocol
callback is captured; appended OAuth result parameters are returned unchanged.
The caller validates and parses the result. An empty callback is supported for
existing providers which explicitly use a caller-owned loopback server.

Closing waits for removal of the owned tab and its popup tree. Caller exit
cancels only its flows. Network portal closure leaves owned account flows alive.
The client handles unavailable/restarted services as a failure and ignores late
results. Owned flows disable password-saving prompts and close script popups.

Import `Sailfish.CaptivePortal 1.0 as Portal` for the QML client. A page can own
a `Portal.Flow` directly. Use `Portal.Flows.create(window, window)` when the
initiating page may be destroyed before asynchronous closure completes. The
factory retains the helper's creation context. Set `returnWindow` to the caller's
application window; it is activated after closure, before `closed` is emitted.
Disconnect page-owned event handlers during teardown. Call `open(url, purpose,
callbackUrl)` and `close(reason)`; the helper provides `updated`, `loaded`,
`failed` and `closed` signals. `failed` also reports `unavailable` for D-Bus errors.

Set `Flow.heading` to a caller-translated plain-text heading before opening.
An empty heading (the default) hides the heading row. The service limits headings
to 256 characters; page titles and website content cannot replace the heading.
The original `openFlow` D-Bus method remains supported and supplies no heading.

Legacy network-portal `openUrl(QStringList)` accepts `[url, heading]`.
Omitting the second argument retains the translated network “Sign in” heading;
an explicitly empty second argument hides it. Cold command-line invocation uses
the same positional URL/heading convention as forwarding to a running instance.
Ordinary Sparse URL launches have no heading.

Sparse allows cross-site HTTP(S) links by default. To restore same-origin link
blocking, set this dconf key as the device session user, then fully quit and
restart Sparse:

```sh
dconf write /apps/sailfish-browser/settings/sparse_block_cross_site_links true
```

Set it to `false`, or use `dconf reset` on the same key, to allow cross-site links
again. The setting applies only to Sparse; ordinary captive portal mode retains
its existing same-origin policy and external-browser handoff.

With blocking enabled, outside-origin HTTP(S) links leave the current page open
and show a short notice instead of dispatching to the default browser, which
may itself be Sparse. Recognized authentication callbacks complete without a notice.
Different schemes, ports and subdomains count as different origins. Login forms
may submit across origins, and native-app links retain the external-URL handoff.
This is a click policy, not a restriction on redirects or script-driven
navigation. Owned authentication callbacks are still returned to their caller.

Sparse's bin icon opens a confirmation dialog to clear shared cookies, site data
and saved passwords. Clearing is explicit; there is no clear-on-exit preference
or desktop-mode menu. Optional caller headings remain supported.
