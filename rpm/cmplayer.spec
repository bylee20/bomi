# Presteps for building the rpm in ${HOME}
# Make sure you have the rpm dev tools installed:
# $ yum install @development-tools fedora-packager # Fedora
# $ zypper in rpm-build # OpenSUSE
# $ zypper in http://download.opensuse.org/repositories/devel:/tools/openSUSE_13.1/x86_64/fakeroot-1.20-1.2.x86_64.rpm
# $ zypper in http://download.opensuse.org/repositories/devel:/tools/openSUSE_13.1/noarch/rpmdevtools-8.3-2.1.noarch.rpm
# Then setup your RPM development folders
# $ rpmdev-setuptree
# Copy source tarball (cmplayer-x.x.x-src.tar.gz) into ${HOME}/rpmbuild/SOURCES and
# place this spec file into ${HOME}/rpmbuild/SPECS
# Install build dependencies:
# $ yum-builddep cmplayer.spec # Fedora
# OpenSUSE has no built in tool for installing .spec build dependencies. However, RPM
# can be used to extract dependencies from a SRPM:
# $ rpm -qp --requires cmplayer.src.rpm | xargs sudo zypper in
# Now build the package with rpmbuild:
# $ rpmbuild -ba cmplayer.spec
# Built rpms are in ${HOME}/rpmbuild/RPMS and source RPMs are located in ${HOME}/rpmbuild/SRPMS

# Note that OpenSUSE users will need to have the Packman Essentials and Multimedia repositories enabled
# see http://en.opensuse.org/Additional_package_repositories#Packman for more details.

%define is_fedora %(test -e /etc/fedora-release && echo 1 || echo 0)
%define is_mandrake %(test -e /etc/mandrake-release && echo 1 || echo 0)
%define is_suse %(test -e /etc/SuSE-release && echo 1 || echo 0)

%define name cmplayer
%define version 0.8.6
%define lrelease lrelease
%define gpp_pkg gcc-c++
%define qmake qmake-qt5

%define _prefix /usr

%if %is_fedora
%define distro %(head -1 /etc/fedora-release)
%define lrelease lrelease-qt4
%define qt_dev qt5-qtbase-devel
%endif
%if %is_mandrake
%define distro %(head -1 /etc/mandrake-release)
%endif
%if %is_suse
%define distro %(head -1 /etc/SuSE-release)
%define qt_dev libqt5-qtbase-devel
%endif

Name: %{name}
Summary: A multimedia player
License: GPL
Group: Applications/Multimedia
Version: %{version}

%if %is_fedora
Release: 1%{dist}
Source: %{name}-%{version}-source.tar.gz
Packager: xylosper <darklin20@gmail.com>
Distribution: %{distro}
BuildRoot: %{_tmppath}/%{name}-buildroot

# Distro-specific dependencies
%if %is_fedora
BuildRequires: ffmpeg-devel
BuildRequires: bzip2-devel
BuildRequires: jack-audio-connection-kit-devel
BuildRequires: pulseaudio-libs-devel
BuildRequires: qt5-qtdeclarative-devel
BuildRequires: qt5-qtquickcontrols
BuildRequires: qt5-qtx11extras-devel
%endif

%if %is_suse
BuildRequires: libbz2-devel
BuildRequires: libffmpeg-devel
BuildRequires: libjack-devel
BuildRequires: libpulse-devel
BuildRequires: libQt5Declarative-devel
BuildRequires: libQt5Quick-devel
BuildRequires: libQt5X11Extras-devel
%endif

BuildRequires: %{gpp_pkg} >= 4.8
BuildRequires: %{qt_dev} >= 5.1.1
BuildRequires: glib2-devel
BuildRequires: libass-devel
BuildRequires: libcdio-paranoia-devel
BuildRequires: libchardet-devel
BuildRequires: libdvdread-devel
BuildRequires: libmpg123-devel
BuildRequires: libquvi-devel
BuildRequires: libva-devel
BuildRequires: openal-soft-devel
BuildRequires: portaudio-devel
BuildRequires: xcb-util-devel
BuildRequires: xcb-util-wm-devel
# rpmbuild's automatic dependency handling misses qt5-qtquickcontrols
%if %is_fedora
Requires: qt5-qtquickcontrols
%else
Requires: libqt5-qtquickcontrols
%endif
Prefix: %{_prefix}
Autoreqprov: on

%description
CMPlayer is a Qt-based multimedia player utilizing the MPV video backend.

%prep
%setup -q

%build
make clean
# Build libchardet statically.
./download-libchardet
./build-libchardet
./build-mpv
make QMAKE=%{qmake} PREFIX=%{_prefix} LIBQUIVI_SUFFIX=-0.9 cmplayer

%install
make QMAKE=%{qmake} DEST_DIR=%{?buildroot:%{buildroot}} PREFIX=%{_prefix} LIBQUIVI_SUFFIX=-0.9 install

%clean
rm -rf $RPM_BUILD_ROOT

%post
/usr/bin/update-desktop-database -q
xdg-icon-resource forceupdate --theme hicolor &> /dev/null

%postun
/usr/bin/update-desktop-database -q
xdg-icon-resource forceupdate --theme hicolor &> /dev/null

%files
%attr (-,root,root)
%{_bindir}/*
%{_datadir}/applications/*
%{_datadir}/apps/solid/actions/*
%{_datadir}/%{name}/*
%{_datadir}/icons/hicolor/*/*/%{name}.png

%changelog
* Sun Dec 01 2013 Ben Reedy <thebenj88@gmail.com> - 0.8.6-1
- Updated build instructions for OpenSUSE users.
- Updated build dependencies for OpenSUSE users.

* Sat Nov 30 2013 xylosper <darklin20@gmail.com> - 0.8.6-1
- Upstream Release
- New: new option 'Apply in fullscreen mode only' for 'Hide mouse cursor'
- Fix: hiding mouse cursor with Qt 5.2.0 didn't work (#22)
- Fix: crash when linked with Qt 5.2.0
- Fix: open file dialog will open in the folder where last played is located
- Fix: urlencoded URLs doesn't open when passed as a parameter (issue #18)
- Fix: volume normalizer is broken(issue #17)
