

GLLIB_MAC = -framework GLUT -framework OpenGL -framework Cocoa \
			/Users/xieran/freetype-gl-read-only/freetype-gl.a \
			`freetype-config --libs`

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
else
AVLIB := ${AVLIB_LINUX}
endif

all: encdec_test

%.o: %.c
	gcc -c -o $@ $<

encdec_test: mp4dec.o mp4enc.o encdec_test.o
	gcc -o $@ $^ ${AVLIB}

show4: encdec_test4
	$< 2>log2 >log && ffplay /tmp/out.mp4

clean:
	rm -rf *.o ${TESTS}

