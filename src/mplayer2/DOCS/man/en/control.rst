Interactive Control
===================

MPlayer has a fully configurable, command-driven control layer which allows you
to control MPlayer using keyboard, mouse, joystick or remote control (with
LIRC). The sections below describe the default bindings. Bindings can be
freely reconfigured in the input.conf configuration file.

Key input works in either video playback window or terminal window. Modifier
keys (Alt, Ctrl and Meta, plus Shift for combinations with non-printable
characters like Shift+RIGHT) may work only partially or not at all depending
on the platform and input method. For example, terminal input does not support
modifiers at all, while Linux video outputs using X support arbitrary modifier
combinations.


keyboard control
----------------

LEFT and RIGHT
    Seek backward/forward 10 seconds. These keys will only seek to video
    keyframes, so the actual step may be more than 10 seconds.

UP and DOWN
    Seek forward/backward 1 minute.

PGUP and PGDWN
    Seek forward/backward 10 minutes.

Shift+LEFT and Shift+RIGHT
    Seek backward/forward exactly 1 second using precise seeking (see option
    ``--hr-seek`` for details).

Shift+UP and Shift+DOWN
    Seek forward/backward exactly 5 seconds using precise seeking (see option
    ``--hr-seek`` for details).

[ and ]
    Decrease/increase current playback speed by 10%.

{ and }
    Halve/double current playback speed.

BACKSPACE
    Reset playback speed to normal.

< and >
    Go backward/forward in the playlist.

ENTER
    Go forward in the playlist, even over the end.

HOME and END
    next/previous playtree entry in the parent list

INS and DEL (ASX playlist only)
    next/previous alternative source.

p / SPACE
    Pause (pressing again unpauses).

.
    Step forward. Pressing once will pause movie, every consecutive press will
    play one frame and then go into pause mode again.

q / ESC
    Stop playing and quit.

U
    Stop playing (and quit if ``--idle`` is not used).

\+ and -
    Adjust audio delay by +/- 0.1 seconds.

/ and *
    Decrease/increase volume.

9 and 0
    Decrease/increase volume.

( and )
    Adjust audio balance in favor of left/right channel.

m
    Mute sound.

\_
    Cycle through the available video tracks.

\#
    Cycle through the available audio tracks.

TAB (MPEG-TS and libavformat only)
    Cycle through the available programs.

f
    Toggle fullscreen (see also ``--fs``).

T
    Toggle stay-on-top (see also ``--ontop``).

w and e
    Decrease/increase pan-and-scan range.

o
    Toggle OSD states: none / seek / seek + timer / seek + timer + total time.

d
    Toggle frame dropping states: none / skip display / skip decoding (see
    ``--framedrop`` and ``--hardframedrop``).

v
    Toggle subtitle visibility.

j and J
    Cycle through the available subtitles.

y and g
    Step forward/backward in the subtitle list.

F
    Toggle displaying "forced subtitles".

a
    Toggle subtitle alignment: top / middle / bottom.

x and z
    Adjust subtitle delay by +/- 0.1 seconds.

V
    Toggle subtitle VSFilter aspect compatibility mode. See
    ``--ass-vsfilter-aspect-compat`` for more info.

C (``--capture`` only)
    Start/stop capturing the primary stream.

r and t
    Move subtitles up/down.

i (``--edlout`` mode only)
    Set start or end of an EDL skip and write it out to the given file.

s
    Take a screenshot. The file will contain the original video image only,
    without extra elements like separate subtitles or OSD content.

S
    Start/stop taking video-only screenshots of every new frame drawn.

Alt+s
    Take a screenshot of the current player window contents. The file will
    contain the current contents of the player window: video will be scaled
    to the current window size, and any subtitle or OSD elements will be
    included.

Alt+S
    Start/stop taking screenshots of the player window for every new frame
    drawn.

I
    Show filename on the OSD.

P
    Show progression bar, elapsed time and total duration on the OSD.

! and @
    Seek to the beginning of the previous/next chapter.

D (``--vo=vdpau``, ``--vf=yadif``, ``--vf=kerndeint`` only)
    Activate/deactivate deinterlacer.

A
    Cycle through the available DVD angles.

c
    Change YUV colorspace.

(The following keys are valid only when using a video output that supports the
corresponding adjustment, the software equalizer (``--vf=eq`` or ``--vf=eq2``)
or hue filter (``--vf=hue``).)

1 and 2
    Adjust contrast.

3 and 4
    Adjust brightness.

5 and 6
    Adjust hue.

7 and 8
    Adjust saturation.

(The following keys are valid only when using the corevideo video output
driver.)

command + 0
    Resize movie window to half its original size.

command + 1
    Resize movie window to its original size.

command + 2
    Resize movie window to double its original size.

command + f
    Toggle fullscreen (see also ``--fs``).

command + [ and command + ]
    Set movie window alpha.

(The following keys are valid only when using the sdl video output driver.)

c
    Cycle through available fullscreen modes.

n
    Restore original mode.

(The following keys are valid if you have a keyboard with multimedia keys.)

PAUSE
    Pause.

STOP
    Stop playing and quit.

PREVIOUS and NEXT
    Seek backward/forward 1 minute.

(The following keys are only valid if you compiled with TV or DVB input
support and will take precedence over the keys defined above.)

h and k
    Select previous/next channel.

n
    Change norm.

u
    Change channel list.

(The following keys are only valid if you compiled with dvdnav support: They
are used to navigate the menus.)

keypad 8
    Select button up.

keypad 2
    Select button down.

keypad 4
    Select button left.

keypad 6
    Select button right.

keypad 5
    Return to main menu.

keypad 7
    Return to nearest menu (the order of preference is: chapter->title->root).

keypad ENTER
    Confirm choice.

(The following keys are used for controlling TV teletext. The data may come
from either an analog TV source or an MPEG transport stream.)

X
    Switch teletext on/off.

Q and W
    Go to next/prev teletext page.

mouse control
-------------

button 3 and button 4
    Seek backward/forward 1 minute.

button 5 and button 6
    Decrease/increase volume.

joystick control
----------------

left and right
    Seek backward/forward 10 seconds.

up and down
    Seek forward/backward 1 minute.

button 1
    Pause.

button 2
    Toggle OSD states: none / seek / seek + timer / seek + timer + total time.

button 3 and button 4
    Decrease/increase volume.


