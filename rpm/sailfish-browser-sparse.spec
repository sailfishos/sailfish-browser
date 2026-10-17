Name: sailfish-browser-sparse
Summary: Reduced Sailfish sign-in browser
Version: 3.1.0
Release: 1
License: MPLv2.0
Url: https://github.com/sailfishos/sailfish-browser
Source0: %{name}-%{version}.tar.bz2
BuildArch: noarch
Requires: sailfish-captiveportal = %{version}
Requires: desktop-file-utils
Conflicts: sailfish-browser
Conflicts: sailfish-components-webview-qt5

%description
Portal-style HTTP(S) URL handling with persistent sign-in data and transient
flows. Uses the Browser desktop identifier without an application-grid launcher.

%prep
%setup -q

%build

%install
install -D -m 644 sparse/sailfish-browser.desktop %{buildroot}%{_datadir}/applications/sailfish-browser.desktop
install -d %{buildroot}%{_datadir}/dbus-1/services
install -m 644 sparse/*.service %{buildroot}%{_datadir}/dbus-1/services/
install -D -m 644 sparse/60-sparse-allowlist.conf %{buildroot}%{_sysconfdir}/sailjail/config/60-sparse-allowlist.conf
install -D -m 644 sparse/SparseBrowser.permission %{buildroot}%{_sysconfdir}/sailjail/permissions/SparseBrowser.permission

%post
update-desktop-database %{_datadir}/applications || :

%postun
update-desktop-database %{_datadir}/applications || :
if [ "$1" -eq 0 ]; then
    # Reload policy and clear the grant inherited from Sparse on removal.
    systemctl try-restart sailjaild.service || :
fi

%posttrans
# Sailjail reads allowlist configuration at startup.
systemctl try-restart sailjaild.service || :

%files
%license LICENSES/MPL-2.0.txt
# Policy must disappear on removal, rather than survive as a modified config.
%{_sysconfdir}/sailjail/config/60-sparse-allowlist.conf
%{_sysconfdir}/sailjail/permissions/SparseBrowser.permission
%{_datadir}/applications/sailfish-browser.desktop
%{_datadir}/dbus-1/services/org.sailfishos.browser.service
%{_datadir}/dbus-1/services/org.sailfishos.browser.ui.service
