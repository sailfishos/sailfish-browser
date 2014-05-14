Name:       sailfish-browser

Summary:    Sailfish Browser
Version:    1.0.7
Release:    1
Group:      Applications/Internet
License:    MPLv2
Url:        https://github.com/sailfishos/sailfish-browser
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(qt5embedwidget) >= 1.8.9
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(nemotransferengine-qt5)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(qdeclarative5-boostable)
BuildRequires:  qt5-qttools
BuildRequires:  qt5-qttools-linguist
BuildRequires:  gdb
Requires: sailfishsilica-qt5 >= 0.11.8
Requires: jolla-ambient >= 0.3.24
Requires: xulrunner-qt5 >= 29.0.1.9
Requires: embedlite-components-qt5 >= 1.5.5
Requires: sailfish-browser-settings = %{version}
Requires: qt5-plugin-imageformat-ico
Requires: qt5-plugin-imageformat-gif
Requires: qt5-plugin-position-geoclue
Requires: mapplauncherd-booster-silica-qt5
Requires: desktop-file-utils
Requires: qt5-qtgraphicaleffects
Requires: nemo-qml-plugin-contextkit-qt5
Requires: nemo-qml-plugin-connectivity
Requires: sailfish-components-media-qt5

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

# >> setup
# << setup

%build
# >> build pre
# << build pre

%qmake5

make %{?jobs:-j%jobs}

# >> build post
# << build post

%install
rm -rf %{buildroot}
# >> install pre
# << install pre
%qmake5_install

# >> install post
# << install post

%post
# >> post
/usr/bin/update-desktop-database -q
# << post

%files
%defattr(-,root,root,-)
# >> files
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/applications/open-url.desktop
%{_datadir}/%{name}/*
%{_datadir}/translations/sailfish-browser_eng_en.qm
%{_datadir}/dbus-1/services/*.service
# << files

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
