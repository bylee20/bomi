%define is_mandrake %(test -e /etc/mandrake-release && echo 1 || echo 0)
%define is_suse %(test -e /etc/SuSE-release && echo 1 || echo 0)
%define is_fedora %(test -e /etc/fedora-release && echo 1 || echo 0)

%define _topdir %(echo ${HOME})/rpm
%define _tmppath %{_topdir}/tmp
%define _prefix /usr/local
%define _actiondir %{_datadir}/kde4/apps/solid/actions
%define _plugindir %{_libdir}/%{name}/plugins

%define name cmplayer
%define version 0.5.2
%define qmake_cmd qmake
%define lrelease_cmd lrelease
%define common_build_require gcc-c++ libqt4-devel >= 4.6 vlc-devel >= 1.1
%define build_require %{common_build_require}

%if %is_suse
%define distro %(head -1 /etc/SuSE-release)
%endif

Name: %{name}
Summary: A multimedia player
License: GPL
Group: Applications/Multimedia
Version: %{version}
Release: 1
Source: %{name}-%{version}.tar.gz
Packager: xylosper <darklin20@gmail.com>
Distribution: %{distro}
BuildRoot: %{_tmppath}/%{name}-buildroot
BuildRequires: %{build_require}
Prefix: %{_prefix}
Autoreqprov: on

%description
CMPlayer is a media player.

%prep
%setup -q

%build
make QMAKE=%{qmake_cmd} LRELEASE=%{lrelease_cmd} PREFIX=%{_prefix} CMPLAYER_ACTION_PATH=%{_actiondir} CMPLAYER_PLUGIN_PATH=%{_plugindir} -f Makefile.linux cmplayer  

%install
make QMAKE=%{qmake_cmd} LRELEASE=%{lrelease_cmd} DEST_DIR=%{?buildroot:%{buildroot}} PREFIX=%{_prefix} CMPLAYER_ACTION_PATH=%{_actiondir} CMPLAYER_PLUGIN_PATH=%{_plugindir} -f Makefile.linux cmplayer install

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr (-,root,root)
%{_bindir}/%{name}
%{_plugindir}/libcmplayer-vout_plugin.so
%{_plugindir}/libcmplayer-vfilter_plugin.so
%{_plugindir}/libcmplayer-afilter_plugin.so
%{_datadir}/applications/cmplayer.desktop
%{_actiondir}/cmplayer-opendvd.desktop
%{_datadir}/icons/hicolor/16x16/apps/cmplayer.png
%{_datadir}/icons/hicolor/22x22/apps/cmplayer.png
%{_datadir}/icons/hicolor/24x24/apps/cmplayer.png
%{_datadir}/icons/hicolor/32x32/apps/cmplayer.png
%{_datadir}/icons/hicolor/48x48/apps/cmplayer.png
%{_datadir}/icons/hicolor/64x64/apps/cmplayer.png
%{_datadir}/icons/hicolor/128x128/apps/cmplayer.png
%{_datadir}/icons/hicolor/256x256/apps/cmplayer.png
