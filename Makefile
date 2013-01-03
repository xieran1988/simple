
LIB = -framework GLUT -framework OpenGL -framework Cocoa \
			/Users/xieran/freetype-gl-read-only/freetype-gl.a
AVLIB := \
		/usr/lib/libav*.a \
		/usr/lib/libbz2.dylib \
		/usr/lib/libz.dylib \
		/usr/lib/libx264.dylib \
		-framework CoreFoundation \
		-framework VideoDecodeAcceleration \
		-framework QuartzCore

all: fbo 
	./$<

minifbo: minifbo.o
	gcc -o $@ $< $(LIB)

stencil: stencil.o
	gcc -o $@ $< $(LIB)

%.o: %.c
	gcc -c -I /Users/xieran/freetype-gl-read-only/ -o $@ $<

fbo: fbo.o
	gcc -o fbo fbo.o $(LIB) 

simple: simple.c decode.so
	gcc -o simple simple.c $(LIB) decode.so
	./simple

decode: decode.o
	gcc -o $@ $< ${AVLIB}

decode_test: decode
	./$<

decode.o: decode.c decode.h

decode.so: decode.o
	gcc -shared -fPIC -o $@ $< ${AVLIB}

encode: encode.o
	gcc -o $@ $< ${AVLIB}

encode.o: encode.c encode.h

.PHONY: test1

test1: test1.o encode.so decode.so
	gcc -o $@ $^ ${AVLIB}
	./$@

encode.so: encode.o
	gcc -shared -fPIC -o $@ $< ${AVLIB}

clean:
	rm *.o
