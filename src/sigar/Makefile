kern := $(shell uname -s)
kern := $(shell if test $(kern) = "Darwin"; then echo "darwin"; elif test $(kern) = "Linux"; then echo "linux"; else echo "unknown"; fi)

CC := gcc
CFLAGS := -I./include -I./src/os/$(kern)
SRCS := $(wildcard src/*.c) src/os/$(kern)/$(kern)_sigar.c
OBJS := $(patsubst %.c, %.o, $(SRCS))

ifeq ($(kern),darwin)
	CFLAGS := $(CFLAGS) -I/System/Library/Frameworks/CoreFoundation.framework/Headers \
		-I/System/Library/Frameworks/CoreServices.framework/Versions/Current/Frameworks/CarbonCore.framework/Headers -DDARWIN -DDARWIN_HAS_LIBPROC_H
endif

sigar: $(OBJS)
	ar rcvs ../../build/lib/libcmplayer_$@.a $^ 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	find . -name *.o | xargs rm
