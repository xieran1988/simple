
GLLIB_MAC := -framework GLUT -framework OpenGL -framework Cocoa 

FTLIB := `freetype-config --libs`

JPGLIB := /opt/libjpeg-turbo/lib/libjpeg.dylib 

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

A := ar crv 

_all: all

%.o: %.c
	gcc -I$T -I/opt/libjpeg-turbo/include `freetype-config --cflags` -c -o $@ $< 

clean:
	rm -rf *.o *.a test *.so


