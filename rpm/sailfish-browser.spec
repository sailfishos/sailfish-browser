%global min_xulrunner_version 38.8.0.4
%global min_qtmozembed_version 1.13.9
%global min_embedlite_components_version 1.9.4

Name:       sailfish-browser

Summary:    Sailfish Browser
Version:    1.14.17
Release:    1
Group:      Applications/Internet
License:    MPLv2
Url:        https://github.com/sailfishos/sailfish-browser
Source0:    %{name}-%{version}.tar.bz2
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
BuildRequires:  qt5-qttools
BuildRequires:  qt5-qttools-linguist
BuildRequires:  oneshot
BuildRequires:  gtest-devel
BuildRequires:  libgmock-devel

Requires: sailfishsilica-qt5 >= 0.22.13
Requires: jolla-ambient >= 0.7.12
Requires: xulrunner-qt5 >= %{min_xulrunner_version}
Requires: embedlite-components-qt5 >= %{min_embedlite_components_version}
Requires: qtmozembed-qt5 >= %{min_qtmozembed_version}
Requires: sailfish-browser-settings = %{version}
Requires: qt5-plugin-imageformat-ico
Requires: qt5-plugin-imageformat-gif
Requires: qt5-plugin-position-geoclue
Requires: mapplauncherd >= 4.1.17
Requires: mapplauncherd-booster-browser
Requires: desktop-file-utils
Requires: qt5-qtgraphicaleffects
Requires: nemo-qml-plugin-contextkit-qt5
Requires: nemo-qml-plugin-connectivity
Requires: nemo-qml-plugin-policy-qt5 >= 0.0.4
Requires: sailfish-components-media-qt5
Requires: sailfish-components-pickers-qt5 >= 0.1.7

%{_oneshot_requires_post}

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}

%description
Sailfish Web Browser

%package settings
Summary:  Browser plugin for Jolla Settings
License:  MPLv2
Group:    Applications/Internet
Requires: jolla-settings >= 0.11.29

%description settings
Browser plugin for Jolla Settings

%package ts-devel
Summary: Translation source for Sailfish browser
License:   MPLv2
Group:     Applications/Internet

%description ts-devel
Translation source for Sailfish Browser

%package tests
Summary: Tests for Sailfish browser
License:   MPLv2
Group: Applications/Internet
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

%post
/usr/bin/update-desktop-database -q

# Upgrade, count is 2 or higher (depending on the number of versions installed)
if [ "$1" -ge 2 ]; then
%{_bindir}/add-oneshot --user --now browser-cleanup-startup-cache
fi

%{_bindir}/add-oneshot --user --late browser-update-default-data

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/applications/open-url.desktop
%{_datadir}/%{name}/*
%{_datadir}/translations/sailfish-browser_eng_en.qm
%{_datadir}/dbus-1/services/*.service
%{_oneshotdir}/*

%files settings
%defattr(-,root,root,-)
%{_datadir}/jolla-settings/*
%{_libdir}/qt5/qml/org/sailfishos/browser/settings/*
%{_datadir}/translations/settings-sailfish-browser_eng_en.qm

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/sailfish-browser.ts
%{_datadir}/translations/source/settings-sailfish-browser.ts

%files tests
%defattr(-,root,root,-)
%{_datadir}/applications/test-sailfish-browser.desktop
/opt/*
