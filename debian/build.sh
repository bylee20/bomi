#! /bin/bash

ver=$1
distro=$2

if [ -z "$distro" ] || [ -z "$ver" ]; then
  echo "Usage: $0 <version> <distro>"
  exit
fi

if ! [ -e v$ver.tar.gz ]; then
  wget https://github.com/xylosper/bomi/archive/v$ver.tar.gz
  mv v$ver.tar.gz bomi-$ver.tar.gz
  tar xvfz bomi-$ver.tar.gz
  cd bomi-$ver
  ./download-ffmpeg
  ./download-libchardet
  cd src/mpv
  ./bootstrap.py
  cd ../../..
fi

cd bomi-$ver
rm -rf debian
cp -r ../$distro debian
debuild -S -sa