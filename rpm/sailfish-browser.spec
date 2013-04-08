Name:       sailfish-browser

Summary:    Sailfish Browser
Version:    0.1.4
Release:    1
Group:      Applications/Internet
License:    Prop
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(QtCore)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(qtembedwidget)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(QtOpenGL)
BuildRequires:  pkgconfig(QJson)
Requires: sailfishsilica >= 0.8.6
Requires: xulrunner >= 22.0.1.11
Requires: embedlite-components >= 1.0.11

%description
Sailfish Web Browser


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
/usr/share/applications/sailfish-browser.desktop
/usr/bin/sailfish-browser
/usr/share/sailfish-browser/*
# << files
