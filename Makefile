

GLLIB_MAC = -framework GLUT -framework OpenGL -framework Cocoa \
			/Users/xieran/freetype-gl-read-only/freetype-gl.a \
			`freetype-config --libs`

AVLIB_MAC := \
		/usr/lib/libav*.a \
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
else
AVLIB := ${AVLIB_LINUX}
endif

TESTS := encdec_test1 encdec_test2 encdec_test3

all: ${TESTS}

%.o: %.c
	gcc -c -o $@ $<

encdec_test1: mp4dec.o x264enc.o encdec_test1.o
	gcc -o $@ $^ ${AVLIB}

encdec_test2: mp4dec.o x264enc.o mp4enc.o encdec_test2.o
	gcc -o $@ $^ ${AVLIB}

encdec_test3: encdec_test3.o
	gcc -o $@ $^ ${AVLIB}

clean:
	rm -rf *.o ${TESTS}

