Name:       sailfish-browser

Summary:    Sailfish Browser
Version:    0.2.0
Release:    1
Group:      Applications/Internet
License:    Prop
Url:        https://bitbucket.org/jolla/ui-sailfish-browser
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(QtCore)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(qtembedwidget)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(QtOpenGL)
BuildRequires:  pkgconfig(QJson)
BuildRequires:  pkgconfig(QtDBus)
Requires: sailfishsilica >= 0.8.6
Requires: xulrunner >= 22.0.1.11
Requires: embedlite-components >= 1.0.11

%description
Sailfish Web Browser

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
%{_datadir}/translations/*.qm
%{_datadir}/dbus-1/services/*.service
# << files

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/sailfish-browser.ts

%files tests
%defattr(-,root,root,-)
%{_datadir}/applications/test-sailfish-browser.desktop
/opt/*
