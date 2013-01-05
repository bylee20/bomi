Video Output Drivers
====================

Video output drivers are interfaces to different video output facilities. The
syntax is:

--vo=<driver1[:suboption1[=value]:...],driver2,...[,]>
    Specify a priority list of video output drivers to be used.

If the list has a trailing ',' MPlayer will fall back on drivers not contained
in the list. Suboptions are optional and can mostly be omitted.

*NOTE*: See ``--vo=help`` for a list of compiled-in video output drivers.

*EXAMPLE*:

    ``--vo=gl,xv,``
        Try the gl driver, then the Xv driver, then others.
    ``--vo=directx:noaccel``
        Uses the DirectX driver with acceleration features turned off.

Available video output drivers are:

xv (X11 only)
    Uses the XVideo extension to enable hardware accelerated playback. This is
    the most compatible VO on X, but may be low quality, and has issues with
    OSD and subtitle display.
    For information about what colorkey is used and how it is drawn run
    MPlayer with ``-v`` option and look out for the lines tagged with ``[xv
    common]`` at the beginning.

    adaptor=<number>
        Select a specific XVideo adaptor (check xvinfo results).
    port=<number>
        Select a specific XVideo port.
    ck=<cur|use|set>
        Select the source from which the colorkey is taken (default: cur).

        cur
          The default takes the colorkey currently set in Xv.
        use
          Use but do not set the colorkey from MPlayer (use the ``--colorkey``
          option to change it).
        set
          Same as use but also sets the supplied colorkey.

    ck-method=<man|bg|auto>
        Sets the colorkey drawing method (default: man).

        man
          Draw the colorkey manually (reduces flicker in some cases).
        bg
          Set the colorkey as window background.
        auto
          Let Xv draw the colorkey.

x11 (X11 only)
    Shared memory video output driver without hardware acceleration that works
    whenever X11 is present.

vdpau (X11 only)
    Uses the VDPAU interface to display and optionally also decode video.
    Hardware decoding is used with ``--vc=ffmpeg12vdpau``,
    ``--vc=ffwmv3vdpau``, ``--vc=ffvc1vdpau``, ``--vc=ffh264vdpau`` or
    ``--vc=ffodivxvdpau``.

    sharpen=<-1-1>
        For positive values, apply a sharpening algorithm to the video, for
        negative values a blurring algorithm (default: 0).
    denoise=<0-1>
        Apply a noise reduction algorithm to the video (default: 0, no noise
        reduction).
    deint=<-4-4>
        Select deinterlacing mode (default: -3). Positive values choose mode
        and enable deinterlacing. Corresponding negative values select the
        same deinterlacing mode, but do not enable deinterlacing on startup
        (useful in configuration files to specify what mode will be enabled by
        the "D" key). All modes respect ``--field-dominance``.

        0
            same as -3
        1
            Show only first field, similar to ``--vf=field``.
        2
            Bob deinterlacing, similar to ``--vf=tfields=1``.
        3
            motion adaptive temporal deinterlacing. May lead to A/V desync
            with slow video hardware and/or high resolution.
        4
            motion adaptive temporal deinterlacing with edge-guided spatial
            interpolation. Needs fast video hardware.
    chroma-deint
        Makes temporal deinterlacers operate both on luma and chroma (default).
        Use no-chroma-deint to solely use luma and speed up advanced
        deinterlacing. Useful with slow video memory.
    pullup
        Try to apply inverse telecine, needs motion adaptive temporal
        deinterlacing.
    hqscaling=<0-9>
        0
            Use default VDPAU scaling (default).
        1-9
            Apply high quality VDPAU scaling (needs capable hardware).
    fps=<number>
        Override autodetected display refresh rate value (the value is needed
        for framedrop to allow video playback rates higher than display
        refresh rate, and for vsync-aware frame timing adjustments). Default 0
        means use autodetected value. A positive value is interpreted as a
        refresh rate in Hz and overrides the autodetected value. A negative
        value disables all timing adjustment and framedrop logic.
    composite-detect
        NVIDIA's current VDPAU implementation behaves somewhat differently
        under a compositing window manager and does not give accurate frame
        timing information. With this option enabled, the player tries to
        detect whether a compositing window manager is active. If one is
        detected, the player disables timing adjustments as if the user had
        specified fps=-1 (as they would be based on incorrect input). This
        means timing is somewhat less accurate than without compositing, but
        with the composited mode behavior of the NVIDIA driver there is no
        hard playback speed limit even without the disabled logic. Enabled by
        default, use no-composite-detect to disable.
    queuetime_windowed=<number> and queuetime_fs=<number>
        Use VDPAU's presentation queue functionality to queue future video
        frame changes at most this many milliseconds in advance (default: 50).
        See below for additional information.
    output_surfaces=<2-15>
        Allocate this many output surfaces to display video frames (default:
        3). See below for additional information.

    Using the VDPAU frame queueing functionality controlled by the queuetime
    options makes MPlayer's frame flip timing less sensitive to system CPU
    load and allows MPlayer to start decoding the next frame(s) slightly
    earlier which can reduce jitter caused by individual slow-to-decode
    frames. However the NVIDIA graphics drivers can make other window behavior
    such as window moves choppy if VDPAU is using the blit queue (mainly
    happens if you have the composite extension enabled) and this feature is
    active. If this happens on your system and it bothers you then you can set
    the queuetime value to 0 to disable this feature. The settings to use in
    windowed and fullscreen mode are separate because there should be less
    reason to disable this for fullscreen mode (as the driver issue shouldn't
    affect the video itself).

    You can queue more frames ahead by increasing the queuetime values and the
    output_surfaces count (to ensure enough surfaces to buffer video for a
    certain time ahead you need at least as many surfaces as the video has
    frames during that time, plus two). This could help make video smoother in
    some cases. The main downsides are increased video RAM requirements for
    the surfaces and laggier display response to user commands (display
    changes only become visible some time after they're queued). The graphics
    driver implementation may also have limits on the length of maximum
    queuing time or number of queued surfaces that work well or at all.

sdl (SDL only, buggy/outdated)
    Highly platform independent SDL (Simple Directmedia Layer) library video
    output driver. Since SDL uses its own X11 layer, MPlayer X11 options do
    not have any effect on SDL. Note that it has several minor bugs
    (``--vm``/``--no-vm`` is mostly ignored, ``--fs`` behaves like ``--no-vm``
    should, window is in top-left corner when returning from fullscreen,
    panscan is not supported, ...).

    driver=<driver>
        Explicitly choose the SDL driver to use.
    (no-)forcexv
        Use XVideo through the sdl video output driver (default: forcexv).
    (no-)hwaccel
        Use hardware accelerated scaler (default: hwaccel).

direct3d (Windows only) (BETA CODE!)
    Video output driver that uses the Direct3D interface (useful for Vista).

directx (Windows only)
    Video output driver that uses the DirectX interface.

    noaccel
        Turns off hardware acceleration. Try this option if you have display
        problems.

corevideo (Mac OS X 10.6 and later)
    Mac OS X CoreVideo video output driver. Uses the CoreVideo APIs to fill
    PixelBuffers and generate OpenGL textures from them (useful as a fallback
    for vo_gl).

gl
    OpenGL video output driver, simple version. Video size must be smaller
    than the maximum texture size of your OpenGL implementation. Intended to
    work even with the most basic OpenGL implementations, but also makes use
    of newer extensions, which allow support for more colorspaces and direct
    rendering. For optimal speed try adding the options ``--dr=-noslices``

    The code performs very few checks, so if a feature does not work, this
    might be because it is not supported by your card/OpenGL implementation
    even if you do not get any error message. Use ``glxinfo`` or a similar
    tool to display the supported OpenGL extensions.

    (no-)ati-hack
        ATI drivers may give a corrupted image when PBOs are used (when using
        ``--dr`` or `force-pbo`). This option fixes this, at the expense of
        using a bit more memory.
    (no-)force-pbo
        Always uses PBOs to transfer textures even if this involves an extra
        copy. Currently this gives a little extra speed with NVidia drivers
        and a lot more speed with ATI drivers. May need ``--no-slices`` and
        the ati-hack suboption to work correctly.
    (no-)scaled-osd
        Changes the way the OSD behaves when the size of the window changes
        (default: disabled). When enabled behaves more like the other video
        output drivers, which is better for fixed-size fonts. Disabled looks
        much better with FreeType fonts and uses the borders in fullscreen
        mode. Does not work correctly with ass subtitles (see ``--ass``), you
        can instead render them without OpenGL support via ``--vf=ass``.
    osdcolor=<0xAARRGGBB>
        Color for OSD (default: 0x00ffffff, corresponds to non-transparent
        white).
    rectangle=<0,1,2>
        Select usage of rectangular textures which saves video RAM, but often
        is slower (default: 0).

        0
            Use power-of-two textures (default).
        1
            Use the ``GL_ARB_texture_rectangle`` extension.
        2
            Use the ``GL_ARB_texture_non_power_of_two`` extension. In some
            cases only supported in software and thus very slow.

    swapinterval=<n>
        Minimum interval between two buffer swaps, counted in displayed frames
        (default: 1). 1 is equivalent to enabling VSYNC, 0 to disabling VSYNC.
        Values below 0 will leave it at the system default. This limits the
        framerate to (horizontal refresh rate / n). Requires
        ``GLX_SGI_swap_control`` support to work. With some (most/all?)
        implementations this only works in fullscreen mode.
    ycbcr
        Use the ``GL_MESA_ycbcr_texture`` extension to convert YUV to RGB. In
        most cases this is probably slower than doing software conversion to
        RGB.
    yuv=<n>
        Select the type of YUV to RGB conversion. The default is
        auto-detection deciding between values 0 and 2.

        0
            Use software conversion. Compatible with all OpenGL versions.
            Provides brightness, contrast and saturation control.
        1
            Same as 2. This used to use nVidia-specific extensions, which
            didn't provide any advantages over using fragment programs, except
            possibly on very ancient graphic cards. It produced a gray-ish
            output, which is why it has been removed.
        2
            Use a fragment program. Needs the ``GL_ARB_fragment_program``
            extension and at least three texture units. Provides brightness,
            contrast, saturation and hue control.
        3
            Use a fragment program using the POW instruction. Needs the
            ``GL_ARB_fragment_program`` extension and at least three texture
            units. Provides brightness, contrast, saturation, hue and gamma
            control. Gamma can also be set independently for red, green and
            blue. Method 4 is usually faster.
        4
            Use a fragment program with additional lookup. Needs the
            ``GL_ARB_fragment_program`` extension and at least four texture
            units. Provides brightness, contrast, saturation, hue and gamma
            control. Gamma can also be set independently for red, green and
            blue.
        5
            Use ATI-specific method (for older cards). This uses an
            ATI-specific extension (``GL_ATI_fragment_shader`` - not
            ``GL_ARB_fragment_shader``!). At least three texture units are
            needed. Provides saturation and hue control. This method is fast
            but inexact.
        6
            Use a 3D texture to do conversion via lookup. Needs the
            ``GL_ARB_fragment_program extension`` and at least four texture
            units. Extremely slow (software emulation) on some (all?) ATI
            cards since it uses a texture with border pixels. Provides
            brightness, contrast, saturation, hue and gamma control. Gamma can
            also be set independently for red, green and blue. Speed depends
            more on GPU memory bandwidth than other methods.

    lscale=<n>
        Select the scaling function to use for luminance scaling. Only valid
        for yuv modes 2, 3, 4 and 6.

        0
            Use simple linear filtering (default).
        1
            Use bicubic B-spline filtering (better quality). Needs one
            additional texture unit. Older cards will not be able to handle
            this for chroma at least in fullscreen mode.
        2
            Use cubic filtering in horizontal, linear filtering in vertical
            direction. Works on a few more cards than method 1.
        3
            Same as 1 but does not use a lookup texture. Might be faster on
            some cards.
        4
            Use experimental unsharp masking with 3x3 support and a default
            strength of 0.5 (see `filter-strength`).
        5
            Use experimental unsharp masking with 5x5 support and a default
            strength of 0.5 (see `filter-strength`).

    cscale=<n>
        Select the scaling function to use for chrominance scaling. For
        details see `lscale`.
    filter-strength=<value>
        Set the effect strength for the `lscale`/`cscale` filters that support
        it.
    noise-strength=<value>
        Set how much noise to add. 0 to disable (default), 1.0 for a level
        suitable for dithering to 6 bit.
    stereo=<value>
        Select a method for stereo display. You may have to use ``--aspect`` to
        fix the aspect value. Experimental, do not expect too much from it.

        0
            Normal 2D display
        1
            Convert side by side input to full-color red-cyan stereo.
        2
            Convert side by side input to full-color green-magenta stereo.
        3
            Convert side by side input to quadbuffered stereo. Only supported
            by very few OpenGL cards.

    The following options are only useful if writing your own fragment
    programs.

    customprog=<filename>
        Load a custom fragment program from <filename>. See
        ``TOOLS/edgedect.fp`` for an example.
    customtex=<filename>
        Load a custom "gamma ramp" texture from <filename>. This can be used
        in combination with yuv=4 or with the customprog option.
    (no-)customtlin
        If enabled (default) use ``GL_LINEAR`` interpolation, otherwise use
        ``GL_NEAREST`` for customtex texture.
    (no-)customtrect
        If enabled, use texture_rectangle for customtex texture. Default is
        disabled.
    (no-)mipmapgen
        If enabled, mipmaps for the video are automatically generated. This
        should be useful together with the customprog and the TXB instruction
        to implement blur filters with a large radius. For most OpenGL
        implementations this is very slow for any non-RGB formats. Default is
        disabled.

    Normally there is no reason to use the following options, they mostly
    exist for testing purposes.

    (no-)glfinish
        Call ``glFinish()`` before swapping buffers. Slower but in some cases
        more correct output (default: disabled).
    (no-)manyfmts
        Enables support for more (RGB and BGR) color formats (default:
        enabled). Needs OpenGL version >= 1.2.
    slice-height=<0-...>
        Number of lines copied to texture in one piece (default: 0). 0 for
        whole image.

        *NOTE*: If YUV colorspace is used (see `yuv` suboption), special rules
        apply: If the decoder uses slice rendering (see ``--no-slices``), this
        setting has no effect, the size of the slices as provided by the
        decoder is used. If the decoder does not use slice rendering, the
        default is 16.
    (no-)osd
        Enable or disable support for OSD rendering via OpenGL (default:
        enabled). This option is for testing; to disable the OSD use
        ``--osdlevel=0`` instead.
    backend=<sys>
        auto
            auto-select (default)
        cocoa
            Cocoa/OSX
        win
            Win32/WGL
        x11
            X11/GLX

gl3
    OpenGL video output driver, extended version. The requires an OpenGL 3
    capable graphics driver. (Note: this is only because of developer pedantry.
    The dependency on actual OpenGL 3 features is rather low.)

    It supports extended scaling methods, dithering and color management.
    It tries to use sane defaults for good quality output.

    Note that some cheaper LCDs do dithering that gravely interferes with
    vo_gl3's dithering. Disabling dithering with ``dither-depth=-1`` helps.

    Sometimes you can achieve better quality or performance by changing the
    fbo-format sub-option to ``rgb16f``, ``rgb32f`` or ``rgb``. (Known
    problems include Mesa/Intel not accepting ``rgb16``, Mesa sometimes not
    being compiled with float texture support, and some OSX setups being very
    slow with ``rgb16``, but fast with ``rgb32f``.)

    lscale=<filter>
        Set the scaling filter. Possible choices:
            bilinear
            bicubic_fast
            sharpen3
            sharpen5
            hanning
            hamming
            hermite
            quadric
            bicubic
            kaiser
            catmull_rom
            mitchell
            spline16
            spline36
            gaussian
            sinc2
            sinc3
            sinc4
            lanczos2
            lanczos3
            lanczos4
            blackman2
            blackman3
            blackman4

        bilinear
            Bilinear hardware texture filtering (fastest, mid-quality).

        lanczos2
            Lanczos scaling with radius=2. Provides a good quality and speed.
            This is the default.

        lanczos3
            Lanczos with radius=3.

        bicubic_fast
            Bicubic filter. Has a blurring effect on the image, even if no
            scaling is done.

        sharpen3
            Unsharp masking (sharpening) with radius=3 and a default strength
            of 0.5 (see ``lparam1``).

        sharpen5
            Unsharp masking (sharpening) with radius=5 and a default strength
            of 0.5 (see ``lparam1``).

        mitchell
            Mitchell-Netravali. The ``b`` and ``c`` parameters can be set with
            ``lparam1`` and ``lparam2``. Both are set to 1/3 by default.

    lparam1=<value>
        Set filter parameters. Ignored if the filter is not tunable. These are
        unset by default, and use the filter specific default if applicable.

    lparam2=<value>
        See ``lparam1``.

    stereo=<value>
        Select a method for stereo display. You may have to use ``--aspect`` to
        fix the aspect value. Experimental, do not expect too much from it.

        0
            Normal 2D display
        1
            Convert side by side input to full-color red-cyan stereo.
        2
            Convert side by side input to full-color green-magenta stereo.
        3
            Convert side by side input to quadbuffered stereo. Only supported
            by very few OpenGL cards.

    srgb
        Enable gamma-correct scaling by working in linear light. This
        makes use of sRGB textures and framebuffers.
        This option forces the options 'indirect' and 'gamma'.
        NOTE: for YUV colorspaces, gamma 2.2 is assumed. RGB input is always
        assumed to be in sRGB.
        This option is not really useful, as gamma-correct scaling does not
        have much influence on typical video playback.

    pbo
        Enable use of PBOs. This is faster, but can sometimes lead to
        sporadic and temporary image corruption.

    dither-depth=<n>
        Positive non-zero values select the target bit depth. Default: 0.

        \-1
            Disable any dithering done by mplayer.
        0
            Automatic selection. If output bit depth can't be detected,
            8 bits per component are assumed.
        8
            Dither to 8 bit output.

        Note that dithering will always be disabled if the bit depth
        of the video is lower or equal to the detected dither-depth.
        If color management is enabled, input depth is assumed to be
        16 bits, because the 3D LUT output is 16 bit wide.

        Note that the depth of the connected video display device can not be
        detected. Often, LCD panels will do dithering on their own, which
        conflicts with vo_gl3's dithering, and leads to ugly output.

    debug
        Check for OpenGL errors, i.e. call glGetError(). Also request a
        debug OpenGL context (which does nothing with current graphics drivers
        as of this writing).


    swapinterval=<n>
        Interval in displayed frames between two buffer swaps.
        1 is equivalent to enable VSYNC, 0 to disable VSYNC.

    no-scale-sep
        When using a separable scale filter for luma, usually two filter
        passes are done. This is often faster. However, it forces
        conversion to RGB in an extra pass, so it can actually be slower
        if used with fast filters on small screen resolutions. Using
        this options will make rendering a single operation.
        Note that chroma scalers are always done as 1-pass filters.

    cscale=<n>
        As lscale but for chroma (2x slower with little visible effect).
        Note that with some scaling filters, upscaling is always done in
        RGB. If chroma is not subsampled, this option is ignored, and the
        luma scaler is used instead. Setting this option is often useless.

    no-fancy-downscaling
        When using convolution based filters, don't extend the filter
        size when downscaling. Trades downscaling performance for
        reduced quality.

    no-npot
        Force use of power-of-2 texture sizes. For debugging only.
        Borders will be distorted due to filtering.

    glfinish
        Call glFinish() before swapping buffers

    backend=<sys>
        auto
            auto-select (default)
        cocoa
            Cocoa/OSX
        win
            Win32/WGL
        x11
            X11/GLX

    indirect
        Do YUV conversion and scaling as separate passes. This will
        first render the video into a video-sized RGB texture, and
        draw the result on screen. The luma scaler is used to scale
        the RGB image when rendering to screen. The chroma scaler
        is used only on YUV conversion, and only if the video uses
        chroma-subsampling.
        This mechanism is disabled on RGB input.
        Specifying this option directly is generally useful for debugging only.

    fbo-format=<fmt>
        Selects the internal format of any FBO textures used.
        fmt can be one of: rgb, rgba, rgb8, rgb16, rgb16f, rgb32f
        Default: rgb16.

    gamma
        Always enable gamma control. (Disables delayed enabling.)

    force-gl2
        Create a legacy GL context. This will randomly malfunction
        if the proper extensions are not supported.

    icc-profile=<file>
        Load an ICC profile and use it to transform linear RGB to
        screen output. Needs LittleCMS2 support compiled in.

    icc-cache=<file>
        Store and load the 3D LUT created from the ICC profile in
        this file. This can be used to speed up loading, since
        LittleCMS2 can take a while to create the 3D LUT.
        Note that this file contains an uncompressed LUT. Its size depends on
        the ``3dlut-size``, and can be very big.

    icc-intent=<value>
        0
            perceptual
        1
            relative colorimetric
        2
            saturation
        3
            absolute colorimetric (default)

    3dlut-size=<r>x<g>x<b>
        Size of the 3D LUT generated from the ICC profile in each
        dimension. Default is 128x256x64.
        Sizes must be a power of two, and 256 at most.

null
    Produces no video output. Useful for benchmarking.

caca
    Color ASCII art video output driver that works on a text console.

directfb
    Play video using the DirectFB library.

    (no-)input
        Use the DirectFB instead of the MPlayer keyboard code (default:
        enabled).
    buffermode=single|double|triple
        Double and triple buffering give best results if you want to avoid
        tearing issues. Triple buffering is more efficient than double
        buffering as it does not block MPlayer while waiting for the vertical
        retrace. Single buffering should be avoided (default: single).
    fieldparity=top|bottom
        Control the output order for interlaced frames (default: disabled).
        Valid values are top = top fields first, bottom = bottom fields first.
        This option does not have any effect on progressive film material like
        most MPEG movies are. You need to enable this option if you have
        tearing issues or unsmooth motions watching interlaced film material.
    layer=N
        Will force layer with ID N for playback (default: -1 - auto).
    dfbopts=<list>
        Specify a parameter list for DirectFB.

v4l2 (requires Linux 2.6.22+ kernel)
    Video output driver for V4L2 compliant cards with built-in hardware MPEG
    decoder. See also the lavc video filter.

    <device>
        Explicitly choose the MPEG decoder device name to use (default:
        ``/dev/video16``).
    <output>
        Explicitly choose the TV-out output to be used for the video signal.

md5sum
    Calculate MD5 sums of each frame and write them to a file. Supports RGB24
    and YV12 colorspaces. Useful for debugging.

    outfile=<value>
        Specify the output filename (default: ``./md5sums``).

yuv4mpeg
    Transforms the video stream into a sequence of uncompressed YUV 4:2:0
    images and stores it in a file (default: ``./stream.yuv``). The format is
    the same as the one employed by mjpegtools, so this is useful if you want
    to process the video with the mjpegtools suite. It supports the YV12
    format. If your source file has a different format and is interlaced, make
    sure to use ``--vf=scale=::1`` to ensure the conversion uses interlaced
    mode. You can combine it with the ``--fixed-vo`` option to concatenate
    files with the same dimensions and fps value.

    interlaced
        Write the output as interlaced frames, top field first.
    interlaced_bf
        Write the output as interlaced frames, bottom field first.
    file=<filename>
        Write the output to <filename> instead of the default ``stream.yuv``.

    *NOTE*: If you do not specify any option the output is progressive (i.e.
    not interlaced).

gif89a
    Output each frame into a single animated GIF file in the current
    directory. It supports only RGB format with 24 bpp and the output is
    converted to 256 colors.

    <fps>
        Float value to specify framerate (default: 5.0).
    <output>
        Specify the output filename (default: ``./out.gif``).

    *NOTE*: You must specify the framerate before the filename or the
    framerate will be part of the filename.

    *EXAMPLE*: ``mplayer video.nut --vo=gif89a:fps=15:output=test.gif``

jpeg
    Output each frame into a JPEG file in the current directory. Each file
    takes the frame number padded with leading zeros as name.

    [no]progressive
        Specify standard or progressive JPEG (default: noprogressive).
    [no]baseline
        Specify use of baseline or not (default: baseline).
    optimize=<0-100>
        optimization factor (default: 100)
    smooth=<0-100>
        smooth factor (default: 0)
    quality=<0-100>
        quality factor (default: 75)
    outdir=<dirname>
        Specify the directory to save the JPEG files to (default: ``./``).
    subdirs=<prefix>
        Create numbered subdirectories with the specified prefix to save the
        files in instead of the current directory.
    maxfiles=<value> (subdirs only)
        Maximum number of files to be saved per subdirectory. Must be equal to
        or larger than 1 (default: 1000).

pnm
    Output each frame into a PNM file in the current directory. Each file
    takes the frame number padded with leading zeros as name. It supports PPM,
    PGM and PGMYUV files in both raw and ASCII mode. See also ``pnm(5)``,
    ``ppm(5)`` and ``pgm(5)``.

    ppm
        Write PPM files (default).
    pgm
        Write PGM files.
    pgmyuv
        Write PGMYUV files. PGMYUV is like PGM, but it also contains the U and
        V plane, appended at the bottom of the picture.
    raw
        Write PNM files in raw mode (default).
    ascii
        Write PNM files in ASCII mode.
    outdir=<dirname>
        Specify the directory to save the PNM files to (default: ``./``).
    subdirs=<prefix>
        Create numbered subdirectories with the specified prefix to save the
        files in instead of the current directory.
    maxfiles=<value> (subdirs only)
        Maximum number of files to be saved per subdirectory. Must be equal to
        or larger than 1 (default: 1000).

png
    Output each frame into a PNG file in the current directory. Each file
    takes the frame number padded with leading zeros as name. 24bpp RGB and
    BGR formats are supported.

    z=<0-9>
        Specifies the compression level. 0 is no compression, 9 is maximum
        compression.
    alpha
        Create PNG files with an alpha channel. Note that MPlayer in general
        does not support alpha, so this will only be useful in some rare
        cases.

tga
    Output each frame into a Targa file in the current directory. Each file
    takes the frame number padded with leading zeros as name. The purpose of
    this video output driver is to have a simple lossless image writer to use
    without any external library. It supports the BGR[A] color format, with
    15, 24 and 32 bpp. You can force a particular format with the format video
    filter.

    *EXAMPLE*: ``mplayer video.nut --vf=format=bgr15 --vo=tga``

sharedbuffer (Mac OS X 10.6 and later)
    Mac OS X headless video output designed to interact with GUIs. It copies
    image data to a shared buffer so that the data can be read from a GUI and
    rendered.

    It uses the same protocol as MPlayer's ``-vo corevideo:shared_buffer``

    buffer_name=<buffer_name>
        Name of the shared buffer created with shm_open() as well as the name
        of the NSConnection mplayer2 will try to open (default: mplayerosx).
