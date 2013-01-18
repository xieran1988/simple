
include ../../top.mk

all: a.a genpic test wavpic 

a.a: yuv.o yuvjpg.o a.o
	$A $@ $^

wavpic: wavpic.o
	${CC} -g -o $@ $^ $T/av/encdec/a.a ${AVLIB} ${JPGLIB}

genpic: genpic.o yuvjpg.o
	${CC} -o $@ $^ $T/av/encdec/a.a ${AVLIB} ${JPGLIB}

test: yuvjpg.o yuv.o test.o
	${CC} -o $@ $^ ${JPGLIB}

clean: OBJ += wavpic

