

GLLIB_MAC = -framework GLUT -framework OpenGL -framework Cocoa \
			/Users/xieran/freetype-gl-read-only/freetype-gl.a \
			`freetype-config --libs`
AVLIB_MAC := \
		/usr/lib/libav*.a \
		/usr/lib/libbz2.dylib \
		/usr/lib/libz.dylib \
		/usr/lib/libx264.dylib \
		-framework CoreFoundation \
		-framework VideoDecodeAcceleration \
		-framework QuartzCore

AVLIB_LINUX := \
	/usr/lib/x86_64-linux-gnu/libav*.so \
	/usr/lib/x86_64-linux-gnu/libx264.so \
	-lm -lz -lbz2 -lpthread 

AVLIB := ${AVLIB_LINUX}

TESTS := mp4dec_test x264enc_test mp4enc_test

all: ${TESTS}

%.o: %.c
	gcc -c  -o $@ $<

%_test: %.o
	gcc -o $@ $< ${AVLIB}

clean:
	rm -rf *.o ${TESTS}

