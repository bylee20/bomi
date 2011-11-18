%define is_fedora %(test -e /etc/fedora-release && echo 1 || echo 0)
%define is_mandrake %(test -e /etc/mandrake-release && echo 1 || echo 0)
%define is_suse %(test -e /etc/SuSE-release && echo 1 || echo 0)

%define name cmplayer
%define version 0.5.4
%define qmake qmake
%define lrelease lrelease
%define common_build_require gcc-c++ >= 4.6 qt4-devel >= 4.7 vlc-devel >= 1.1
%define build_require %{common_build_require}

%define _topdir %(echo ${HOME})/rpm
%define _tmppath %{_topdir}/tmp
%define _prefix /usr
%define _actiondir %{_datadir}/kde4/apps/solid/actions
%define _plugindir %{_libdir}/%{name}/plugins

%if %is_fedora
%define distro %(head -1 /etc/fedora-release)
%define qmake qmake-qt4
%define lrelease lrelease-qt4
%endif
%if %is_suse
%define distro %(head -1 /etc/SuSE-release)
%endif
%if %is_mandrake
%define distr %(head -1 /etc/mandrake-release)
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
BuildRequires: %{build_require}
Prefix: %{_prefix}
Autoreqprov: on

%description
CMPlayer is a multimedia player.

%prep
%setup -q

%build
make QMAKE=%{qmake} LRELEASE=%{lrelease} PREFIX=%{_prefix} all

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

#%defattr (-,root,root)
#%{_bindir}/%{name}
#%{_plugindir}/libcmplayer-vout_plugin.so
#%{_plugindir}/libcmplayer-vfilter_plugin.so
#%{_plugindir}/libcmplayer-afilter_plugin.so
#%{_datadir}/applications/cmplayer.desktop
#%{_actiondir}/cmplayer-opendvd.desktop
#%{_datadir}/icons/hicolor/16x16/apps/cmplayer.png
#%{_datadir}/icons/hicolor/22x22/apps/cmplayer.png
#%{_datadir}/icons/hicolor/24x24/apps/cmplayer.png
#%{_datadir}/icons/hicolor/32x32/apps/cmplayer.png
#%{_datadir}/icons/hicolor/48x48/apps/cmplayer.png
#%{_datadir}/icons/hicolor/64x64/apps/cmplayer.png
#%{_datadir}/icons/hicolor/128x128/apps/cmplayer.png
#%{_datadir}/icons/hicolor/256x256/apps/cmplayer.png
