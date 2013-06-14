Name:       sailfish-browser

Summary:    Sailfish Browser
Version:    0.3.5
Release:    1
Group:      Applications/Internet
License:    Prop
Url:        https://bitbucket.org/jolla/ui-sailfish-browser
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(QtCore)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(qtembedwidget) >= 1.0.1
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(QtOpenGL)
BuildRequires:  pkgconfig(QJson)
BuildRequires:  pkgconfig(QtDBus)
BuildRequires:  pkgconfig(nemotransferengine)
Requires: sailfishsilica >= 0.8.34
Requires: jolla-ambient >= 0.1.26
Requires: xulrunner >= 24.0.1.5
Requires: embedlite-components >= 1.2.0
Requires: sailfish-browser-settings = %{version}

%description
Sailfish Web Browser

%package settings
Summary:  Browser plugin for Jolla Settings
License:  Prop
Group:    Applications/Internet
Requires: jolla-settings >= 0.11.29

%description settings
Browser plugin for Jolla Settings

%package ts-devel
Summary: Translation source for Sailfish browser
License:   Prop
Group:     Applications/Internet

%description ts-devel
Translation source for Sailfish Browser

%package tests
Summary: Tests for Sailfish browser
License:   Prop
Group: Applications/Internet
Requires:   %{name} = %{version}-%{release}
Requires:   qtest-qml

%description tests
Unit tests and additional data needed for functional tests

%prep
%setup -q -n %{name}-%{version}

# >> setup
# << setup

%build
# >> build pre
# << build pre

%qmake 

make %{?jobs:-j%jobs}

# >> build post
# << build post

%install
rm -rf %{buildroot}
# >> install pre
# << install pre
%qmake_install

# >> install post
# << install post


%files
%defattr(-,root,root,-)
# >> files
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/%{name}/*
%{_datadir}/translations/sailfish-browser_eng_en.qm
%{_datadir}/dbus-1/services/*.service
%{_libdir}/mozembedlite/chrome/embedlite/content/*.js
# << files

%files settings
%defattr(-,root,root,-)
%{_datadir}/jolla-settings/*
%{_libdir}/qt4/imports/org/sailfishos/browser/settings/*
%{_datadir}/translations/settings-sailfish-browser_eng_en.qm

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/sailfish-browser.ts
%{_datadir}/translations/source/settings-sailfish-browser.ts

%files tests
%defattr(-,root,root,-)
%{_datadir}/applications/test-sailfish-browser.desktop
/opt/*
