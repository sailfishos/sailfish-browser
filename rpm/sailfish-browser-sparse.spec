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

%post
update-desktop-database %{_datadir}/applications || :

%postun
update-desktop-database %{_datadir}/applications || :

%files
%license LICENSES/MPL-2.0.txt
%{_datadir}/applications/sailfish-browser.desktop
%{_datadir}/dbus-1/services/org.sailfishos.browser.service
%{_datadir}/dbus-1/services/org.sailfishos.browser.ui.service
