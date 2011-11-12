kern := $(shell uname -s)
os := $(shell if test $(kern) = "Darwin"; then echo "osx"; elif test $(kern) = "Linux"; then echo "linux"; else echo "unknown"; fi)
cflags := -I. -std=gnu99 -lvlc -lvlccore
ifeq ($(os),osx)
	VLC_INCLUDE_PATH ?= /Applications/VLC.app/Contents/MacOS/include
	VLC_LIB_PATH ?= /Applications/VLC.app/Contents/MacOS/lib
	VLC_PLUGINS_PATH ?= /Applications/VLC.app/Contents/MacOS/plugins
	cflags := $(cflags) -I$(VLC_INCLUDE_PATH) -L$(VLC_LIB_PATH) -dynamiclib \
		-isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
	post_build = \
		install_name_tool -change @loader_path/lib/libvlc.5.dylib \
			@loader_path/../lib/libvlc.5.dylib $@.dylib && \
		install_name_tool -change @loader_path/lib/libvlccore.4.dylib \
			@loader_path/../lib/libvlccore.4.dylib $@.dylib
	lib_ext := dylib
else
	cflags := $(cflags) -shared -pthread
	ifeq ($(shell uname -m),x86_64)
		cflags := $(cflags) -fPIC	
	endif
	lib_ext := so
endif

#	otool -L $@.dylib | grep lib/libvlc | sed 
do_build = gcc $(cflags) $@.c -o $@.$(lib_ext) && $(post_build)

vlc-plugins: libcmplayer-vout_plugin libcmplayer-vfilter_plugin libcmplayer-afilter_plugin

libcmplayer-vout_plugin:
	$(do_build)

libcmplayer-vfilter_plugin:
	$(do_build)
	
libcmplayer-afilter_plugin:
	$(do_build)

clean:
	rm -f libcmplayer-*_plugin.$(lib_ext)
