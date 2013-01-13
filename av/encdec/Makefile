
include ../../top.mk

all: a.a test

a.a: dec.o enc.o
	$A $@ $^

test: dec.o enc.o test.o
	${CC} -o $@ $^ ${AVLIB} $T/av/util/a.a


