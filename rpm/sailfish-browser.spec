Name:       sailfish-browser

Summary:    Sailfish Browser
Version:    0.0.1
Release:    1
Group:      System/Applications
License:    Prop
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(QtCore)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(qtembedwidget)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(QtOpenGL)

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
