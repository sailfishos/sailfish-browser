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
For HTTP(S) callbacks, an explicit default port (80 or 443 respectively) matches
an omitted port; other ports remain distinct.
The caller validates and parses the result. An empty callback is supported for
existing providers which explicitly use a caller-owned loopback server.

Closing waits for removal of the owned tab and its popup tree. Caller exit
cancels only its flows. Network portal closure leaves owned account flows alive.
The client handles unavailable/restarted services as a failure and ignores late
results. Owned flows disable password-saving prompts and close script popups.

Import `Sailfish.CaptivePortal 1.0 as Portal` for the QML client. A page can own
a `Portal.PortalFlow` directly. Use `Portal.Flows.create(window, window)` when the
initiating page may be destroyed before asynchronous closure completes. The
factory retains the helper's creation context. Set `returnWindow` to the caller's
application window; it is activated after closure, before `closed` is emitted.
Disconnect page-owned event handlers during teardown. Call `open(url, purpose,
callbackUrl)` and `close(reason)`; the helper provides `updated`, `loaded`,
`failed` and `closed` signals. `failed` also reports `unavailable` for D-Bus errors.

Set `PortalFlow.heading` to a caller-translated plain-text heading before opening.
An empty heading (the default) hides the heading row. The service rejects headings longer than 256 UTF-16 code units with
`InvalidArgs`; page titles and website content cannot replace the heading.
The original `openFlow` D-Bus method remains supported and supplies no heading.

Legacy network-portal `openUrl(QStringList)` accepts `[url, heading]`.
Omitting the second argument retains the translated network “Sign in” heading;
an explicitly empty second argument hides it. Cold command-line invocation uses
the same positional URL/heading convention as forwarding to a running instance.
Ordinary Sparse URL launches have no heading.

Sparse's bin icon opens a confirmation dialog to clear shared cookies, site data
and saved passwords. Clearing is explicit; there is no clear-on-exit preference
or desktop-mode menu. Optional caller headings remain supported.
