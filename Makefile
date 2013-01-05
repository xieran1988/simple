
LIB = -framework GLUT -framework OpenGL -framework Cocoa \
			/Users/xieran/freetype-gl-read-only/freetype-gl.a \
			`freetype-config --libs`
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
	gcc -c  -o $@ $<

fbo: fbo.o
	gcc -o fbo fbo.o $(LIB) 

simple: simple.c decode.so ft.so
	gcc -I /Users/xieran/freetype-gl-read-only/ -o simple simple.c $(LIB) decode.so ft.so

decode: decode.o
	gcc -o $@ $< ${AVLIB}

decode.o: decode.c decode.h

decode.so: decode.o
	gcc -shared -fPIC -o $@ $< ${AVLIB}

encode: encode.o
	gcc -o $@ $< ${AVLIB}

encode.o: encode.c encode.h

encode.so: encode.o
	gcc -shared -fPIC -o $@ $< ${AVLIB}

ft.o: ft.c
	gcc `freetype-config --cflags` -c -o $@ $< 

ft.so: ft.o
	gcc -shared -fPIC -o $@ $< ${LIB}

.PHONY: test1 testfont

testdecode: decode
	./$<

test1: test1.o encode.so decode.so
	gcc -o $@ $^ ${AVLIB}
	./$@

testsimple: simple
	./simple

testfont:
	gcc `freetype-config --cflags` -DTEST -o testfont ft.c ${LIB}
	./testfont

clean:
	rm -rf *.o *.so simple encode decode fbo test1

