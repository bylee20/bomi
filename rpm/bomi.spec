# Presteps for building the rpm in ${HOME}
# Make sure you have the rpm dev tools installed:
# $ yum install @development-tools fedora-packager # Fedora
# $ zypper in rpm-build # OpenSUSE
# $ zypper in http://download.opensuse.org/repositories/devel:/tools/openSUSE_13.1/x86_64/fakeroot-1.20-1.2.x86_64.rpm
# $ zypper in http://download.opensuse.org/repositories/devel:/tools/openSUSE_13.1/noarch/rpmdevtools-8.3-2.1.noarch.rpm
# Then setup your RPM development folders
# $ rpmdev-setuptree
# Copy source tarball (bomi-x.x.x-src.tar.gz) into ${HOME}/rpmbuild/SOURCES and
# place this spec file into ${HOME}/rpmbuild/SPECS
# Install build dependencies:
# $ yum-builddep bomi.spec # Fedora
# OpenSUSE has no built in tool for installing .spec build dependencies. However, RPM
# can be used to extract dependencies from a SRPM:
# $ rpm -qp --requires bomi.src.rpm | xargs sudo zypper in
# Now build the package with rpmbuild:
# $ rpmbuild -ba bomi.spec
# Built rpms are in ${HOME}/rpmbuild/RPMS and source RPMs are located in ${HOME}/rpmbuild/SRPMS

# Note that OpenSUSE users will need to have the Packman Essentials and Multimedia repositories enabled
# see http://en.opensuse.org/Additional_package_repositories#Packman for more details.

Name:       bomi
Summary:    A multimedia player
License:    GPLv2
Group:      Applications/Multimedia
Version:    0.8.16
Release:    1%{?dist}
Url:        http://bomi.github.io/
Source0:    https://github.com/xylosper/%{name}/releases/download/v%{version}/%{name}-%{version}-source.tar.gz

# Distro-specific dependencies
%if 0%{?fedora}
BuildRequires:  ffmpeg-devel
BuildRequires:  bzip2-devel
BuildRequires:  jack-audio-connection-kit-devel
BuildRequires:  libvdpau-devel
BuildRequires:  pulseaudio-libs-devel
BuildRequires:  qt5-qtbase-devel >= 5.1.1
BuildRequires:  qt5-qtdeclarative-devel
BuildRequires:  qt5-qtquickcontrols
BuildRequires:  qt5-qtx11extras-devel
%endif

%if 0%{?suse_version}
BuildRequires:  libbz2-devel
BuildRequires:  libffmpeg-devel
BuildRequires:  libjack-devel
BuildRequires:  libpulse-devel
BuildRequires:  libqt5-qtbase-devel >= 5.1.1
BuildRequires:  libQt5Declarative-devel
BuildRequires:  libQt5Quick-devel
BuildRequires:  libQt5X11Extras-devel
%endif

BuildRequires:  gcc-c++ >= 4.8
BuildRequires:  glib2-devel
BuildRequires:  libass-devel
BuildRequires:  libbluray-devel
BuildRequires:  libcdio-paranoia-devel
BuildRequires:  libchardet-devel
BuildRequires:  libdvdnav-devel
BuildRequires:  libdvdread-devel
BuildRequires:  libmpg123-devel
BuildRequires:  libva-devel
BuildRequires:  python
BuildRequires:  xcb-util-devel
BuildRequires:  xcb-util-wm-devel
# rpm's automatic dependency handling misses qt5-qtquickcontrols
%if 0%{?fedora}
Requires: qt5-qtquickcontrols
%else
Requires: libqt5-qtquickcontrols
%endif

%description
bomi is a Qt-based multimedia player utilizing the MPV video back-end.

%prep
%setup -q

%build
# Build libchardet statically.
./download-libchardet
./build-libchardet
./configure --prefix=/usr \
 --enable-jack \ 
 --enable-pulseaudio \
 --enable-cdda \
 --mandir=%{_mandir}
make %{?_smp_mflags}

%install
make DEST_DIR=%{?buildroot:%{buildroot}} install

%post
/usr/bin/update-desktop-database -q
xdg-icon-resource forceupdate --theme hicolor &> /dev/null

%postun
/usr/bin/update-desktop-database -q
xdg-icon-resource forceupdate --theme hicolor &> /dev/null

%files
%{_bindir}/*
%{_datadir}/applications/*
%{_datadir}/apps/solid/actions/*
%{_datadir}/%{name}/*
%{_datadir}/icons/hicolor/*/*/%{name}.png

%doc COPYING.txt CHANGES.txt GPL.txt ICON-AUTHORS.txt ICON-COPYING.txt MPL.txt README.md

%changelog
* Tue Sep 30 2014 Ben R <thebenj88@gmail.com> - 0.8.16-1
- Upstream release

* Fri Jun 13 2014 Ben R <thebenj88@gmail.com> - 0.8.14-1
- Upstream release

* Fri Mar 21 2014 Ben R <thebenj88@gmail.com> - 0.8.13-1
- Upstream release
- Added libbluray-devel build dependency

* Sat Mar 08 2014 Ben Reedy <thebenj88@gmail.com> - 0.8.12-1
- Upstream release

* Wed Feb 12 2014 Ben Reedy <thebenj88@gmail.com> - 0.8.11-1
- Upstream release

* Sun Feb 09 2014 Ben Reedy <thebenj88@gmail.com> - 0.8.10-1
- Upstream release
- Openal dependency removed; upstream has dropped openal support

* Mon Feb 03 2014 Ben Reedy <thebenj88@gmail.com> - 0.8.9-1
- Upstream release

* Sun Feb 02 2014 Ben Reedy <thebenj88@gmail.com> - 0.8.8-1
- Upstream release

* Sat Feb 01 2014 Ben Reedy <thebenj88@gmail.com> - 0.8.7-1
- Upstream release

* Thu Dec 12 2013 Ben Reedy <thebenj88@gmail.com> - 0.8.6-2
- Updated dependencies

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
