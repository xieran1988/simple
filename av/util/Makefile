
include ../../top.mk

all: genpic test

genpic: genpic.o yuvjpg.o
	${CC} -o $@ $^ $T/av/encdec/a.a ${AVLIB} ${JPGLIB}

test: yuvjpg.o yuv.o test.o
	${CC} -o $@ $^ ${JPGLIB}

