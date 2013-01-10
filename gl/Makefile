
include ../top.mk

all: gl_test

gl_test: gl_test.o
	${CC} -o $@ $^ ${GLLIB} \
		$T/gl/vid/vid.so $T/av/av.so

