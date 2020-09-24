%global min_xulrunner_version 45.8.1.1
%global min_qtmozembed_version 1.14.7
%global min_embedlite_components_version 1.20.0
%global min_sailfishwebengine_version 1.0.6
%global min_systemsettings_version 0.5.25

Name:       sailfish-browser

Summary:    Sailfish Browser
Version:    1.17.11
Release:    1
Group:      Applications/Internet
License:    MPLv2.0
Url:        https://github.com/sailfishos/sailfish-browser
Source0:    %{name}-%{version}.tar.bz2
Source1:    %{name}.privileges
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(qt5embedwidget) >= %{min_qtmozembed_version}
BuildRequires:  pkgconfig(systemsettings) >= %{min_systemsettings_version}
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
BuildRequires:  gtest-devel
BuildRequires:  libgmock-devel
BuildRequires:  pkgconfig(vault) >= 1.0.1

Requires: sailfishsilica-qt5 >= 1.1.107
Requires: sailfish-content-graphics
Requires: xulrunner-qt5 >= %{min_xulrunner_version}
Requires: embedlite-components-qt5 >= %{min_embedlite_components_version}
Requires: qtmozembed-qt5 >= %{min_qtmozembed_version}
Requires: sailfish-browser-settings = %{version}
Requires: sailfish-components-webview-qt5 >= %{min_sailfishwebengine_version}
Requires: sailfish-components-webview-qt5-popups >= %{min_sailfishwebengine_version}
Requires: sailfish-components-webview-qt5-pickers >= %{min_sailfishwebengine_version}
Requires: qt5-plugin-imageformat-ico
Requires: qt5-plugin-imageformat-gif
Requires: qt5-plugin-position-geoclue
Requires: mapplauncherd >= 4.1.17
Requires: mapplauncherd-booster-browser
Requires: desktop-file-utils
Requires: qt5-qtgraphicaleffects
Requires: nemo-qml-plugin-connectivity
Requires: nemo-qml-plugin-policy-qt5 >= 0.0.4
Requires: sailfish-policy >= 0.3.31
Requires: jolla-settings-system >= 1.0.70
Requires: libkeepalive >= 1.7.0
Requires: sailfish-components-pickers-qt5 >= 0.1.7
Requires: nemo-qml-plugin-notifications-qt5 >= 1.0.12
Requires: nemo-qml-plugin-systemsettings >= %{min_systemsettings_version}

%{_oneshot_requires_post}

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}

%description
Sailfish Web Browser

%package settings
Summary:  Browser plugin for Jolla Settings
Requires: jolla-settings >= 0.11.29
Requires: jolla-settings-system >= 1.0.70
Requires: sailfish-policy

%description settings
Browser plugin for Jolla Settings

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
rm -rf %{buildroot}
%qmake5_install
chmod +x %{buildroot}/%{_oneshotdir}/*

mkdir -p %{buildroot}/%{_sharedstatedir}/environment/nemo/
cp -f data/70-browser.conf %{buildroot}/%{_sharedstatedir}/environment/nemo/

mkdir -p %{buildroot}%{_datadir}/mapplauncherd/privileges.d
install -m 644 -p %{SOURCE1} %{buildroot}%{_datadir}/mapplauncherd/privileges.d/
ln -s %{name} %{buildroot}%{_bindir}/%{name}-captiveportal

%post
/usr/bin/update-desktop-database -q || :

# Upgrade, count is 2 or higher (depending on the number of versions installed)
if [ "$1" -ge 2 ]; then
    %{_bindir}/add-oneshot --all-users --now browser-cleanup-startup-cache || :
    %{_bindir}/add-oneshot --new-users --all-users --late browser-update-default-data || :
fi

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_bindir}/%{name}-captiveportal
%{_datadir}/applications/%{name}.desktop
%{_datadir}/applications/%{name}-captiveportal.desktop
%{_datadir}/applications/open-url.desktop
%{_datadir}/%{name}
%{_datadir}/translations/%{name}*.qm
%{_datadir}/dbus-1/services/*.service
%{_datadir}/mapplauncherd/privileges.d/*
%{_oneshotdir}/*
# Let main package own import root level
%dir %{_libdir}/qt5/qml/org/sailfishos/browser
%{_sharedstatedir}/environment/nemo/*
%{_libexecdir}/jolla-vault/units/vault-browser
%{_datadir}/jolla-vault/units/Browser.json


%files settings
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/sailfishos/browser/settings
%{_datadir}/jolla-settings/entries/browser.json
%{_datadir}/jolla-settings/pages/browser
%{_datadir}/translations/settings-%{name}_eng_en.qm

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/*.ts

%files tests
%defattr(-,root,root,-)
%{_datadir}/applications/test-%{name}.desktop
/opt/tests/%{name}
