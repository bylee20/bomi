# bomi


## Introduction

bomi is a multimedia player.
It is aimed for easy usage but also powerful features.
It provides various features and convenience functions.
Just install and enjoy it!
There will be already what you expect.
If you don't like, you can configure almost everything.

For more details, please visit [bomi Project Page](http://bomi.github.io).


## Requirements

Items in next list are the name of each package for pkg-config except OpenGL.
Some packages marked with (*) can be in-tree-built.

* Linux
* Qt5 >= 5.2
* OpenGL >= 2.1 with framebuffer object
* FFmpeg >= 2.0 (libav is not supported)
  * libavformat (*)
  * libavcodec (*)
  * libavfilter (*)
  * libpostproc (*)
  * libswresample (*)
  * libswscale (*)
  * libavutil (*)
* chardet (*)
* libmpg123
* libass
* dvdread
* dvdnav
* icu-uc
* xcb xcb-icccm x11
* libva libva-glx libva-x11
* vdpau
* alsa


## Compilation

FFmpeg and chardet packages cannot be prepared easily for some Linux ditros.
In such cases, you can build them with in-tree source.

* In-tree build for FFmpeg
  1. Run `./download-ffmpeg` to download FFmpeg source.
  2. Run `./build-ffmpeg` to build FFmpeg packages.
* In-tree build for chardet
  1. Run `./download-libchardet` to download chardet source.
  2. Run `./build-libchardet` to build chardet package.

In order to build and install bomi, follow next:

0. If you're trying latest source code from Git repository, run `./init-mpv` first.
1. Run `./configure` with proper options. For details, run `./configure --help`.
2. Run `make` to build bomi

You can find built executable in `./build` directory.
If you want to install bomi into specified directory by `--prefix` option, run `make install`.


## Contacts

### [Issue Tracker](https://github.com/xylosper/bomi/issues)
If you have problems or want some features, please report them in English, Korean, or Japanese.

### [E-mail](mailto:darklin20@gmail.com)
If you want to contact me privately, please send me an e-mail. 


## License

bomi is distributed under GPLv2.

Copyright (C) 2014 xylosper

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