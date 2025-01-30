%global rdnn_name io.github.input_leap.input-leap

Name:		input-leap
Version:	master
Release:	master
#Release:	%autorelease
Summary:	Share mouse and keyboard between multiple computers over the network

License:	GPL-2.0-only
URL:		https://github.com/%{name}/%{name}
%undefine _disable_source_fetch
Source:		%{url}/archive/refs/heads/master.zip

BuildRequires:	cmake >= 3.12
BuildRequires:	desktop-file-utils
BuildRequires:	gcc-c++
BuildRequires:	gmock-devel
BuildRequires:	gulrak-filesystem-devel
BuildRequires:	gtest-devel
BuildRequires:	libappstream-glib
BuildRequires:	libcurl-devel
BuildRequires:	openssl-devel
BuildRequires:	cmake(Qt6Core)
BuildRequires:	cmake(Qt6Core5Compat)
BuildRequires:	cmake(Qt6Widgets)
BuildRequires:	cmake(Qt6Network)
BuildRequires:	cmake(Qt6LinguistTools)
BuildRequires:	pkgconfig(avahi-compat-libdns_sd)
BuildRequires:	pkgconfig(glib-2.0)
BuildRequires:	pkgconfig(gio-2.0)
BuildRequires:	pkgconfig(ice)
BuildRequires:	pkgconfig(libei-1.0) >= 0.99.1
BuildRequires:	pkgconfig(libportal) >= 0.8.0
BuildRequires:	pkgconfig(sm)
BuildRequires:	pkgconfig(x11)
BuildRequires:	pkgconfig(xext)
BuildRequires:	pkgconfig(xi)
BuildRequires:	pkgconfig(xinerama)
BuildRequires:	pkgconfig(xkbcommon)
BuildRequires:	pkgconfig(xrandr)
BuildRequires:	pkgconfig(xtst)
Requires:	hicolor-icon-theme

# https://github.com/input-leap/input-leap/issues/1414
Provides:	barrier = %version-%release
Obsoletes:	barrier <= 2.4.0

%description
Input Leap is software that mimics the functionality of a KVM switch, which
historically would allow you to use a single keyboard and mouse to control
multiple computers by physically turning a dial on the box to switch the
machine you're controlling at any given moment.

Input Leap does this in software, allowing you to tell it which machine to
control by moving your mouse to the edge of the screen, or by using a
keypress to switch focus to a different system.


%prep
%autosetup -p1


%build
%cmake \
	-DINPUTLEAP_BUILD_LIBEI=ON \
	-DINPUTLEAP_BUILD_TESTS=ON \
	-DINPUTLEAP_USE_EXTERNAL_GTEST=True \
	%{?flatpak:-DINPUTLEAP_DEPLOY_FLATPAK_SCRIPT=ON} \
	%{nil}
%cmake_build


%install
%cmake_install


%check
%ctest
desktop-file-validate %{buildroot}%{_datadir}/applications/%{rdnn_name}.desktop
appstream-util validate-relax --nonet %{buildroot}%{_datadir}/metainfo/%{rdnn_name}.appdata.xml


%files
%license LICENSE
%doc ChangeLog README.md doc/%{name}.conf.example*
%if 0%{?flatpak}
%{_bindir}/%{name}-flatpak
%endif
%{_bindir}/%{name}c
%{_bindir}/%{name}s
%{_bindir}/%{name}
%{_datadir}/icons/hicolor/scalable/apps/%{rdnn_name}.svg
%{_datadir}/applications/%{rdnn_name}.desktop
%{_datadir}/metainfo/%{rdnn_name}.appdata.xml
%{_mandir}/man1/%{name}c.1*
%{_mandir}/man1/%{name}s.1*


%changelog
%autochangelog

