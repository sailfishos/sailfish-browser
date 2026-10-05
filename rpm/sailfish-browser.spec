%global min_qtmozembed_version 2.0.0
%global min_sailfishwebengine_version 1.8.0

%global captiveportal sailfish-captiveportal

Name:       sailfish-browser

Summary:    Sailfish Browser
Version:    3.1.0
Release:    1
License:    MPLv2.0 and LGPLv2.1
Url:        https://github.com/sailfishos/sailfish-browser
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  lipstick-qt5-devel
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(qt5embedwidget) >= %{min_qtmozembed_version}
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(nemotransferengine-qt5)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(qdeclarative5-boostable)
BuildRequires:  pkgconfig(sailfishwebengine) >= %{min_sailfishwebengine_version}
BuildRequires:  pkgconfig(sailfishpolicy)
BuildRequires:  qt5-qttools
BuildRequires:  qt5-qttools-linguist
BuildRequires:  oneshot
BuildRequires:  pkgconfig(gtest)
BuildRequires:  pkgconfig(gmock)
BuildRequires:  pkgconfig(vault) >= 1.0.1
BuildRequires:  pkgconfig(dsme_dbus_if)

Requires: %{name}-common = %{version}-%{release}
Requires: %{captiveportal} = %{version}-%{release}
Requires: sailjail-launch-approval
Requires: mapplauncherd-booster-browser
Requires: amber-qml-plugin-mpris >= 1.2.10
Obsoletes: sailfish-browser-settings <= 2.3.29
Provides: sailfish-browser-settings > 2.3.29

%{_oneshot_requires_post}

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}

%description
Sailfish Web Browser

%package common
Summary: Shared Sailfish Browser and portal runtime
Requires: jolla-settings >= 0.11.29
Requires: jolla-settings-system >= 1.0.70
Requires: sailfishsilica-qt5 >= 1.2.33
Requires: sailfish-content-graphics
Requires: qtmozembed-qt5 >= %{min_qtmozembed_version}
Requires: sailfish-components-webview-qt5-common >= %{min_sailfishwebengine_version}
Requires: sailfish-components-webview-qt5-popups >= %{min_sailfishwebengine_version}
Requires: sailfish-components-webview-qt5-pickers >= %{min_sailfishwebengine_version}
Requires: qt5-plugin-imageformat-ico
Requires: qt5-plugin-imageformat-gif
Requires: qt5-plugin-position-geoclue
Requires: desktop-file-utils
Requires: qt5-qtgraphicaleffects
Requires: nemo-qml-plugin-policy-qt5 >= 0.0.4
Requires: sailfish-policy >= 0.3.31
Requires: libkeepalive >= 1.7.0
Requires: sailfish-components-pickers-qt5 >= 0.1.7
Requires: nemo-qml-plugin-notifications-qt5 >= 1.0.12
Requires: nemo-qml-plugin-connectivity

%description common
Shared engine integration, QML controls, translations and runtime data.

%package -n %{captiveportal}
Summary: Standalone Sailfish captive portal
Requires: %{name}-common = %{version}-%{release}

%description -n %{captiveportal}
Network sign-in portal, independently installable alongside the full Browser.

%package ts-devel
Summary: Translation source for Sailfish browser

%description ts-devel
Translation source for Sailfish Browser

%package tests
Summary: Tests for Sailfish browser
BuildRequires:  pkgconfig(Qt5Test)
Requires:   %{name} = %{version}-%{release}
Requires:   qt5-qtdeclarative-devel-tools
Requires:   qt5-qtdeclarative-import-qttest
Requires:   mce-tools

%description tests
Unit tests and additional data needed for functional tests

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 -r VERSION=%{version}
%qtc_make %{?_smp_mflags}

%install
%qmake5_install
chmod +x %{buildroot}/%{_oneshotdir}/*

mkdir -p %{buildroot}/%{_sharedstatedir}/environment/nemo/
cp -f data/70-browser.conf %{buildroot}/%{_sharedstatedir}/environment/nemo/

%post
# Upgrade, count is 2 or higher (depending on the number of versions installed)
if [ "$1" -ge 2 ]; then
    %{_bindir}/add-oneshot --all-users --now browser-cleanup-startup-cache || :
    %{_bindir}/add-oneshot --new-users --all-users --late browser-update-default-data || :
fi

%post common -p /sbin/ldconfig
%postun common -p /sbin/ldconfig

%files
%license LICENSES/MPL-2.0.txt
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/%{name}/browser.qml
%{_datadir}/%{name}/pages
%{_datadir}/%{name}/cover
%{_datadir}/translations/settings-%{name}_eng_en.qm
%{_datadir}/dbus-1/services/org.sailfishos.browser.service
%{_datadir}/dbus-1/services/org.sailfishos.browser.ui.service
%{_oneshotdir}/*
%{_userunitdir}/user-session.target.d/50-sailfish-browser.conf
%dir %{_libdir}/qt5/qml/org/sailfishos/browser
%{_sharedstatedir}/environment/nemo/*
%{_libexecdir}/jolla-vault/units/vault-browser
%{_datadir}/jolla-vault/units/Browser.json
%{_libdir}/qt5/qml/org/sailfishos/browser/settings
%{_datadir}/jolla-settings/entries/browser.json
%{_datadir}/jolla-settings/pages/browser

%files common
%license LICENSES/MPL-2.0.txt LICENSES/LGPL-2.1.txt
%{_libdir}/libsailfishbrowser.so.*
%exclude %{_libdir}/libsailfishbrowser.so
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/data
%{_datadir}/%{name}/shared
%dir %{_datadir}/%{captiveportal}
%{_datadir}/%{captiveportal}/shared
%{_datadir}/translations/%{name}*.qm

%files -n %{captiveportal}
%license LICENSES/MPL-2.0.txt
%{_bindir}/%{captiveportal}
%{_datadir}/applications/%{captiveportal}.desktop
%{_datadir}/%{captiveportal}/captiveportal.qml
%{_datadir}/%{captiveportal}/pages
%{_datadir}/translations/%{captiveportal}*.qm
%{_datadir}/dbus-1/services/org.sailfishos.captiveportal.service

%files ts-devel
%{_datadir}/translations/source/*.ts

%files tests
%{_datadir}/applications/test-%{name}.desktop
/opt/tests/%{name}
