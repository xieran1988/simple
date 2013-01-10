
GLLIB_MAC := -framework GLUT -framework OpenGL -framework Cocoa 

FTLIB := `freetype-config --libs`

AVLIB_MAC := \
		/usr/lib/libav*.dylib \
		/usr/lib/libbz2.dylib \
		/usr/lib/libz.dylib \
		/usr/lib/libx264.dylib \
		/usr/lib/libfaac.dylib \
		-framework CoreFoundation \
		-framework VideoDecodeAcceleration \
		-framework QuartzCore

AVLIB_LINUX := \
	/usr/lib/libavformat.so \
	/usr/lib/libavcodec.so \
	/usr/lib/libavutil.so \
	/usr/lib/libavdevice.so \
	/usr/lib/libavfilter.so \
	/usr/lib/x86_64-linux-gnu/libx264.so \
	-lm -lz -lbz2 -lpthread -lswscale

ifeq ($(shell uname -s),Darwin)
AVLIB := ${AVLIB_MAC}
GLLIB := ${GLLIB_MAC}
else
AVLIB := ${AVLIB_LINUX}
endif

T := $(dir $(lastword $(MAKEFILE_LIST)))

SO := ${CC} -shared -fPIC 

_all: all

%.o: %.c
	gcc -I$T `freetype-config --cflags` -c -o $@ $< 

clean:
	rm -rf *.o *_test *.so

