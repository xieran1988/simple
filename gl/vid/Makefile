
include ../../top.mk

all: vid.so

vid.so: vid.o
	${SO} -o $@ $^ ${GLLIB} $T/gl/shader/shader.so

