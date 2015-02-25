# bomi


## Introduction

bomi is a multimedia player formerly known as CMPlayer,
which is aimed for easy usage but also provides various powerful features and convenience functions.
Just install and enjoy it! There will be already what you expect.
If you don't like, you can configure almost everything. 

For more details, please visit [bomi Project Page](http://bomi-player.github.io).

## Requirements

In order to build bomi, you need next tools:

* `g++` or `clang` which supports C++14
* `pkg-config`
* `python`
* `git` if you try to build from git repository

You have to prepare next libraries, too:

* Qt5 >= 5.2
* OpenGL >= 2.1 with framebuffer object support
* FFmpeg (libav is not supported) (*)
  * libavformat >= 55.12.0 (*)
  * libavcodec >= 55.34.1 (*)
  * libavutil >= 52.48.101 (*)
  * libavfilter (*)
  * libswresample (*)
  * libswscale (*)
* chardet (*)
* libmpg123
* libass
* dvdread dvdnav
* libbluray
* icu-uc
* xcb xcb-icccm x11
* libva libva-glx libva-x11
* vdpau
* alsa

Each item corresponds to its package name for `pkg-config` command except Qt and OpenGL.
Some packages marked with (*) can be in-tree-built.

## Compilation

In the below description, `$` means that you have to input the command in termnal/console
where source code exists.

### Get source code

At first, prepare the source code.

* Download the latest source code tarball and unpack
* Or, clone the git repository if you want

### In-tree build packages

FFmpeg and chardet packages cannot be prepared easily for some Linux ditributions.
For such case, you can build them with in-tree source.

If you don't need in-tree build of FFmpeg and chardet, skip this section.

* To build FFmpeg in-tree, run next:
```
$ ./download-ffmpeg
$ ./build-ffmpeg
```

* To build chardet in-tree, run next:
```
$. /download-libchardet
$. /build-libchardet
```

### Build bomi

If you have any problem when building, please check Troubleshooting section.
It may be helpful to check what you can configure using next command:
```
$ ./configure --help
```

#### Test purpose
If you want to try bomi without install, run next commands in order to build bomi:
```
$ ./configure
$ make
```
The executable will be located at `./build/bomi` in source code directory.

#### Install into system

To install bomi, you have to decide the path to install.
It can be spcified by `--prefix` option.
By default, `--prefix=/usr/local` will be applied if you don't specify it which results to locate the executable at `/usr/local/bin/bomi` and other files (skins, translations, etc.) under `/usr/local/share`
For instance, if you want to install into a directory named `bomi` in your home directory, run next:
```
$ ./configure --prefix=${HOME}/bomi
$ make
$ make install
```
You will find the executable at `bomi/bin/bomi` in your home directory.

#### For package builders

Usually, when build a package, you need to specify the fake root system when run `make install`.
This can be accomplished by giving `DEST_DIR` option to `make install`.
Here's a snippet from `PKGBUILD` for Arch Linux as an example:
```bash
build() {
  cd "$srcdir/$pkgname-$pkgver"
  ./configure --prefix=/usr --enable-jack --enable-cdda
  make
}

package() {
  cd "$srcdir/$pkgname-$pkgver"
  make DEST_DIR=$pkgdir install
}
```
where `$pkgdir` is the fake root system. `jack` and `cdda` support is also enabled in this example.

## Contacts

### [Issue Tracker](https://github.com/xylosper/bomi/issues)
If you have problems or want some features, please report them in English, Korean, or Japanese.

### [E-mail](mailto:darklin20@gmail.com)
If you want to contact me privately, please send me an e-mail. 

## License

bomi is distributed under GPLv2.

Copyright (C) 2015 Lee, Byoung-young A.K.A. xylosper

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.