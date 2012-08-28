# presteps build rpm in ${HOME}
# $ mkdir -p ${HOME}/rpm/{BUILD,BUILDROOT,RPMS/$ARCH,RPMS/noarch,SOURCES,SRPMS,SPECS,tmp}
# $ echo -e "%_topdir\t%(echo \${HOME})/rpm\n%_tmppath\t%(echo \${HOME})/rpm/tmp\n" >> ~/.rpmmacros
# to build rpm from spec
# copy source tarboll (cmplayer-x.x.x-src.tar.gz) into ${HOME}/rpm/SOURCES and
# $ rpmbuild -ba cmplayer.spec
# built rpms are in ${HOME}/rpm/RPMS and ${HOME}/rpm/SRPMS

%define is_fedora %(test -e /etc/fedora-release && echo 1 || echo 0)
%define is_mandrake %(test -e /etc/mandrake-release && echo 1 || echo 0)
%define is_suse %(test -e /etc/SuSE-release && echo 1 || echo 0)

%define name cmplayer
%define version 0.6.1
%define qmake qmake
%define lrelease lrelease
%define gpp_pkg gcc-c++
%define qt_dev libqt4-devel

%define _prefix /usr
%define _actiondir %{_datadir}/kde4/apps/solid/actions
%define _plugindir %{_libdir}/%{name}/plugins

%if %is_fedora
%define distro %(head -1 /etc/fedora-release)
%define qmake qmake-qt4
%define lrelease lrelease-qt4
%define qt_dev qt-devel
%endif
%if %is_mandrake
%define distr %(head -1 /etc/mandrake-release)
%endif
%if %is_suse
%define distro %(head -1 /etc/SuSE-release)
%define 
%endif

Name: %{name}
Summary: A multimedia player
License: GPL
Group: Applications/Multimedia
Version: %{version}
Release: 1
Source: %{name}-%{version}-src.tar.gz
Packager: xylosper <darklin20@gmail.com>
Distribution: %{distro}
BuildRoot: %{_tmppath}/%{name}-buildroot
BuildRequires: %{gpp_pkg} >= 4.7 %{qt_dev} >= 4.8
Prefix: %{_prefix}
Autoreqprov: on

%description
CMPlayer is a multimedia player.

%prep
%setup -q

%build
./build-libchardet
./build-sigar
./build-libav
./build-mplayer2
make QMAKE=%{qmake} LRELEASE=%{lrelease} PREFIX=%{_prefix} cmplayer

%install
make QMAKE=%{qmake} LRELEASE=%{lrelease} PREFIX=%{_prefix} DEST_DIR=%{?buildroot:%{buildroot}} install

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%attr (-,root,root) /*
