========
mplayer2
========

------------
movie player
------------

:Manual section: 1


Synopsis
========

| **mplayer** [options] [file|URL|playlist|-]
| **mplayer** [options] file1 [specific options] [file2] [specific options]
| **mplayer** [options] {group of files and options} [group-specific options]
| **mplayer** [br]://[title][/device] [options]
| **mplayer** [dvd|dvdnav]://[title|[start\_title]-end\_title][/device] [options]
| **mplayer** \vcd://track[/device]
| **mplayer** \tv://[channel][/input_id] [options]
| **mplayer** radio://[channel|frequency][/capture] [options]
| **mplayer** \pvr:// [options]
| **mplayer** \dvb://[card\_number@]channel [options]
| **mplayer** \mf://[filemask|\@listfile] [-mf options] [options]
| **mplayer** [cdda|cddb]://track[-endtrack][:speed][/device] [options]
| **mplayer** \cue://file[:track] [options]
| **mplayer** [file|mms[t]|http|http\_proxy|rt[s]p|ftp|udp|unsv|icyx|noicyx|smb]:// [user:pass\@]URL[:port] [options]
| **mplayer** \sdp://file [options]
| **mplayer** \mpst://host[:port]/URL [options]
| **mplayer** \tivo://host/[list|llist|fsid] [options]


Description
===========

**mplayer** is a movie player for Linux (runs on many other platforms and CPU
architectures, see the documentation). It supports a wide variety of video
file formats, audio and video codecs, and subtitle types. Special input URL
types are available to read input from a variety of sources other than disk
files. Depending on platform, a variety of different video and audio output
methods are supported.

Usage examples to get you started quickly can be found at the end of this man
page.


.. include:: control.rst


Usage
=====

Every *flag* option has a *no-flag* counterpart, e.g. the opposite of the
``--fs`` option is ``--no-fs``. ``--fs=yes`` is same as ``--fs``, ``--fs=no``
is the same as ``--no-fs``.

If an option is marked as *(XXX only)*, it will only work in combination with
the *XXX* option or if *XXX* is compiled in.

| *NOTE*: The suboption parser (used for example for ``--ao=pcm`` suboptions)
  supports a special kind of string-escaping intended for use with external
  GUIs.
| It has the following format:
| %n%string\_of\_length\_n
| *EXAMPLES*:
| `mplayer --ao pcm:file=%10%C:test.wav test.avi`
| Or in a script:
| `mplayer --ao pcm:file=%\`expr length "$NAME"\`%"$NAME" test.avi`


Configuration Files
===================

You can put all of the options in configuration files which will be read every
time MPlayer is run. The system-wide configuration file 'mplayer.conf' is in
your configuration directory (e.g. ``/etc/mplayer`` or
``/usr/local/etc/mplayer``), the user specific one is ``~/.mplayer/config``.
User specific options override system-wide options and options given on the
command line override either. The syntax of the configuration files is
``option=<value>``, everything after a *#* is considered a comment. Options
that work without values can be enabled by setting them to *yes* or *1* or
*true* and disabled by setting them to *no* or *0* or *false*. Even suboptions
can be specified in this way.

You can also write file-specific configuration files. If you wish to have a
configuration file for a file called 'movie.avi', create a file named
'movie.avi.conf' with the file-specific options in it and put it in
``~/.mplayer/``. You can also put the configuration file in the same directory
as the file to be played, as long as you give the ``--use-filedir-conf``
option (either on the command line or in your global config file). If a
file-specific configuration file is found in the same directory, no
file-specific configuration is loaded from ``~/.mplayer``. In addition, the
``--use-filedir-conf`` option enables directory-specific configuration files.
For this, MPlayer first tries to load a mplayer.conf from the same directory
as the file played and then tries to load any file-specific configuration.

*EXAMPLE MPLAYER CONFIGURATION FILE:*

| # Use gl3 video output by default.
| vo=gl3
| # I love practicing handstands while watching videos.
| flip=yes
| # Decode multiple files from PNG,
| # start with mf://filemask
| mf=type=png:fps=25
| # Eerie negative images are cool.
| vf=eq2=1.0:-0.8


Profiles
========

To ease working with different configurations profiles can be defined in the
configuration files. A profile starts with its name between square brackets,
e.g. *[my-profile]*. All following options will be part of the profile. A
description (shown by ``--profile=help``) can be defined with the profile-desc
option. To end the profile, start another one or use the profile name
*default* to continue with normal options.

*EXAMPLE MPLAYER PROFILE:*

| [protocol.dvd]
| profile-desc="profile for dvd:// streams"
| vf=pp=hb/vb/dr/al/fd
| alang=en
|
| [protocol.dvdnav]
| profile-desc="profile for dvdnav:// streams"
| profile=protocol.dvd
| mouse-movements=yes
| nocache=yes
|
| [extension.flv]
| profile-desc="profile for .flv files"
| flip=yes


Options
=======

.. include:: options.rst

.. include:: ao.rst

.. include:: vo.rst

.. include:: af.rst

.. include:: vf.rst


Environment Variables
=====================

There are a number of environment variables that can be used to control the
behavior of MPlayer.

``MPLAYER_CHARSET`` (see also ``--msgcharset``)
    Convert console messages to the specified charset (default: autodetect). A
    value of "noconv" means no conversion.

``MPLAYER_HOME``
    Directory where MPlayer looks for user settings.

``MPLAYER_LOCALEDIR``
    Directory where MPlayer looks for gettext translation files (if enabled).

``MPLAYER_VERBOSE`` (see also ``-v`` and ``--msglevel``)
    Set the initial verbosity level across all message modules (default: 0).
    The resulting verbosity corresponds to that of ``--msglevel=5`` plus the
    value of ``MPLAYER_VERBOSE``.

libaf:
    ``LADSPA_PATH``
        If ``LADSPA_PATH`` is set, it searches for the specified file. If it
        is not set, you must supply a fully specified pathname.

        FIXME: This is also mentioned in the ladspa section.

libdvdcss:
    ``DVDCSS_CACHE``
        Specify a directory in which to store title key values. This will
        speed up descrambling of DVDs which are in the cache. The
        ``DVDCSS_CACHE`` directory is created if it does not exist, and a
        subdirectory is created named after the DVD's title or manufacturing
        date. If ``DVDCSS_CACHE`` is not set or is empty, libdvdcss will use
        the default value which is ``${HOME}/.dvdcss/`` under Unix and
        ``C:\Documents and Settings\$USER\Application Data\dvdcss\`` under
        Win32. The special value "off" disables caching.

    ``DVDCSS_METHOD``
        Sets the authentication and decryption method that libdvdcss will use
        to read scrambled discs. Can be one of title, key or disc.

        key
           is the default method. libdvdcss will use a set of calculated
           player keys to try and get the disc key. This can fail if the drive
           does not recognize any of the player keys.

        disc
           is a fallback method when key has failed. Instead of using player
           keys, libdvdcss will crack the disc key using a brute force
           algorithm. This process is CPU intensive and requires 64 MB of
           memory to store temporary data.

        title
           is the fallback when all other methods have failed. It does not
           rely on a key exchange with the DVD drive, but rather uses a crypto
           attack to guess the title key. On rare cases this may fail because
           there is not enough encrypted data on the disc to perform a
           statistical attack, but on the other hand it is the only way to
           decrypt a DVD stored on a hard disc, or a DVD with the wrong region
           on an RPC2 drive.

    ``DVDCSS_RAW_DEVICE``
        Specify the raw device to use. Exact usage will depend on your
        operating system, the Linux utility to set up raw devices is raw(8)
        for instance. Please note that on most operating systems, using a raw
        device requires highly aligned buffers: Linux requires a 2048 bytes
        alignment (which is the size of a DVD sector).

    ``DVDCSS_VERBOSE``
        Sets the libdvdcss verbosity level.

        :0: Outputs no messages at all.
        :1: Outputs error messages to stderr.
        :2: Outputs error messages and debug messages to stderr.

    ``DVDREAD_NOKEYS``
        Skip retrieving all keys on startup. Currently disabled.

    ``HOME``
        FIXME: Document this.

osdep:
    ``TERM``
        FIXME: Document this.

libvo:
    ``DISPLAY``
        FIXME: Document this.

    ``FRAMEBUFFER``
        FIXME: Document this.

    ``HOME``
        FIXME: Document this.

libmpdemux:

    ``HOME``
        FIXME: Document this.

    ``HOMEPATH``
        FIXME: Document this.

    ``http_proxy``
        FIXME: Document this.

    ``LOGNAME``
        FIXME: Document this.

    ``USERPROFILE``
        FIXME: Document this.

libavformat:

    ``http_proxy``
        FIXME: Document this.

    ``no_proxy``
        FIXME: Document this.


Files
=====

``/usr/local/etc/mplayer/mplayer.conf``
    MPlayer system-wide settings

``~/.mplayer/config``
    MPlayer user settings

``~/.mplayer/input.conf``
    input bindings (see ``--input=keylist`` for the full list)

``~/.mplayer/subfont.ttf``
    Fallback font file. Normally unnecessary; only really needed in the
    very unusual case where the player uses a libass library that has
    fontconfig disabled.

``~/.mplayer/DVDkeys/``
    cached CSS keys


Examples
=========================

Quickstart Blu-ray playing:
    - ``mplayer br:////path/to/disc``
    - ``mplayer br:// --bluray-device=/path/to/disc``

Quickstart DVD playing:
    ``mplayer dvd://1``

Play in Japanese with English subtitles:
    ``mplayer dvd://1 --alang=ja --slang=en``

Play only chapters 5, 6, 7:
    ``mplayer dvd://1 --chapter=5-7``

Play only titles 5, 6, 7:
    ``mplayer dvd://5-7``

Play a multiangle DVD:
    ``mplayer dvd://1 --dvdangle=2``

Play from a different DVD device:
    ``mplayer dvd://1 --dvd-device=/dev/dvd2``

Play DVD video from a directory with VOB files:
    ``mplayer dvd://1 --dvd-device=/path/to/directory/``

Copy a DVD title to hard disk, saving to file title1.vob :
    ``mplayer dvd://1 --dumpstream --dumpfile=title1.vob``

Play a DVD with dvdnav from path /dev/sr1:
    ``mplayer dvdnav:////dev/sr1``

Stream from HTTP:
    ``mplayer http://mplayer.hq/example.avi``

Stream using RTSP:
    ``mplayer rtsp://server.example.com/streamName``

Convert subtitles to MPsub format:
    ``mplayer dummy.avi --sub=source.sub --dumpmpsub``

Convert subtitles to MPsub format without watching the movie:
    ``mplayer /dev/zero --rawvideo=pal:fps=xx --demuxer=rawvideo --vc=null --vo=null --noframedrop --benchmark --sub=source.sub --dumpmpsub``

input from standard V4L:
    ``mplayer tv:// --tv=driver=v4l:width=640:height=480:outfmt=i420 --vc=rawi420 --vo=xv``

Play DTS-CD with passthrough:
    ``mplayer --ac=hwdts --rawaudio=format=0x2001 --cdrom-device=/dev/cdrom cdda://``

    You can also use ``--afm=hwac3`` instead of ``--ac=hwdts``. Adjust
    ``/dev/cdrom`` to match the CD-ROM device on your system. If your external
    receiver supports decoding raw DTS streams, you can directly play it via
    ``cdda://`` without setting format, hwac3 or hwdts.

Play a 6-channel AAC file with only two speakers:
    ``mplayer --rawaudio=format=0xff --demuxer=rawaudio --af=pan=2:.32:.32:.39:.06:.06:.39:.17:-.17:-.17:.17:.33:.33 adts_he-aac160_51.aac``

    You might want to play a bit with the pan values (e.g multiply with a
    value) to increase volume or avoid clipping.

checkerboard invert with geq filter:
    ``mplayer --vf=geq='128+(p(X\,Y)-128)*(0.5-gt(mod(X/SW\,128)\,64))*(0.5-gt(mod(Y/SH\,128)\,64))*4'``
