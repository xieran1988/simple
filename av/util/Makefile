
include ../../top.mk

all: a.a genpic test

a.a: yuv.o yuvjpg.o
	$A $@ $^

genpic: genpic.o yuvjpg.o
	${CC} -o $@ $^ $T/av/encdec/a.a ${AVLIB} ${JPGLIB}

test: yuvjpg.o yuv.o test.o
	${CC} -o $@ $^ ${JPGLIB}

