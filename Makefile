

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

show3:
	make && ./encdec_test3 /tmp/out.mp4 2>log2 >log && ffplay /tmp/out.mp4

show2:
	make && ./encdec_test2 2>log2 >log && ffplay /tmp/out.mp4

convs16:
	avconv -f s16le -ar 44.1k -ac 2 -i /tmp/s16.pcm -y /tmp/s16.wav

plays16:
	ffplay -f s16le -ar 44.1k -ac 2 /tmp/s16.pcm


clean:
	rm -rf *.o ${TESTS}

