#! /bin/bash

ver=$1
distro=$2
rel=$3

if [ -z "$distro" ] || [ -z "$ver" ]; then
  echo "Usage: $0 <version> <distro> [<rel>]"
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
  ./download-ffmpeg
  ./download-libchardet
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
 
cp $orig/base/compat .
cp $orig/base/menu .
cp $orig/base/rules .
cp $orig/base/control .
cp ../COPYING.txt copyright

mkdir source
echo "3.0 (native)" > source/format

trusty_bdeps=
trusty_deps=libqt5qml-quickcontrols

utopic_bdeps=
utopic_deps=qml-module-qtquick-controls

bdeps=
for one in $(eval echo \$"$distro"_bdeps); do
    bdeps="$bdeps, $one"
done
deps=
for one in $(eval echo \$"$distro"_deps); do
    deps="$deps, $one"
done

sed -i "s/@bdeps@/$bdeps/g" control
sed -i "s/@deps@/$deps/g" control

debuild -S -sa