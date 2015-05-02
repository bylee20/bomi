#! /bin/bash

ver=$(git describe | sed 's/v\([0-9]\+.[0-9]\+.[0-9]\+\).*/\1/')
distro=$1
rel=$2

if [ -z "$distro" ]; then
  echo "Usage: $0 <distro> [<rel>]"
  exit
fi

if [ -z "$rel" ]; then
  rel=1
fi

orig=$(pwd)

if ! [ -e bomi-$ver.tar.gz ]; then
  wget https://github.com/xylosper/bomi/archive/v$ver.tar.gz
  mv v$ver.tar.gz bomi-$ver.tar.gz
  tar xvfz bomi-$ver.tar.gz
  cd bomi-$ver
  cd src/mpv
  ./bootstrap.py
  ./waf --help &> /dev/null
  mv .waf-*/* .
  sed -i '/^#==>$/,$d' waf
  rmdir .waf-*
fi

cd $orig
cd bomi-$ver
rm -rf debian

mkdir debian
cd debian

printf "bomi ("$ver"ppa"$rel"~"$distro"1) $distro; urgency=low\n\n" >> changelog
printf "  * upstream release\n\n" >> changelog
printf " -- Byoung-young Lee (xylosper) <darklin20@gmail.com>  $(LANG=C date -R)\n" >> changelog

base=ubuntu

cp $orig/$base/compat .
cp $orig/$base/menu .
cp $orig/$base/rules .
cp $orig/$base/control .
cp ../COPYING.txt copyright

mkdir source
echo "3.0 (native)" > source/format

trusty_bdeps=", ffmpeg-bomi, libx264-dev, libvorbis-dev, libvpx-dev, libmp3lame-dev, g++-4.9"
trusty_deps=", qtdeclarative5-qtquick2-plugin, libqt5qml-quickcontrols, qtdeclarative5-quicklayouts-plugin"
trusty_opts="--cc=gcc-4.9"

utopic_bdeps=", ffmpeg-bomi, libx264-dev, libvorbis-dev, libvpx-dev, libmp3lame-dev, g++ (>= 4.9)"
utopic_deps=", qml-module-qtquick2, qml-module-qtquick-controls, qml-module-qtquick-layouts"
utopic_opts=

vivid_bdeps=", g++ (>= 4.9), libsystemd-dev"
vivid_bdeps="$vivid_bdeps, libavformat-ffmpeg-dev (>= 2.4), libavutil-ffmpeg-dev (>= 2.4), libavcodec-ffmpeg-dev (>= 2.4)"
vivid_bdeps="$vivid_bdeps, libswresample-ffmpeg-dev (>= 2.4), libswscale-ffmpeg-dev (>= 2.4), libavfilter-ffmpeg-dev (>= 2.4)"
vivid_deps=", qml-module-qtquick2, qml-module-qtquick-controls, qml-module-qtquick-layouts"
vivid_opts=

bdeps="$(eval echo \$"$distro"_bdeps)"
deps="$(eval echo \$"$distro"_deps)"
opts="$(eval echo \$"$distro"_opts)"

sed -i "s/@bdeps@/$bdeps/g" control
sed -i "s/@deps@/$deps/g" control
sed -i "s/@opts@/$opts/g" rules

debuild -S -sa
