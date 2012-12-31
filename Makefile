
LIB = -framework GLUT -framework OpenGL -framework Cocoa
AVLIB := \
		/usr/lib/libav*.a \
		/usr/lib/libbz2.dylib \
		/usr/lib/libz.dylib \
		-framework CoreFoundation \
		-framework VideoDecodeAcceleration \
		-framework QuartzCore

all: decode.so
	gcc -o simple simple.c $(LIB) decode.so
	./simple

decode: decode.o
	gcc -o decode decode.o ${AVLIB}
	./decode

decode_test: decode
	./decode

decode.o: decode.c decode.h
	gcc -c -o decode.o decode.c

decode.so: decode.o
	gcc -shared -fPIC -o decode.so decode.o ${AVLIB}

