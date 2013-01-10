
include ../../top.mk

all: 
	make clean
	make gl_test

gl_test: test1.o
	${CC} -o $@ $^ ${GLLIB} ${AVLIB} \
		$T/gl/vid/vid.o $T/av/av.a \
		$T/gl/shader/shader.o \
		$T/gl/util/util.o 

