
include ../../top.mk

all: a.a test \
	$T/bin/vidpic \
	$T/bin/wavpic 

a.a: yuv.o yuvjpg.o a.o
	$A $@ $^

$T/bin/wavpic: wavpic.o
	${CC} -g -o $@ $^ $T/av/encdec/a.a ${AVLIB} ${JPGLIB}

$T/bin/vidpic: vidpic.o yuvjpg.o
	${CC} -o $@ $^ $T/av/encdec/a.a ${AVLIB} ${JPGLIB}

test: yuvjpg.o yuv.o test.o
	${CC} -o $@ $^ ${JPGLIB}

clean: OBJ += $T/bin/wavpic $T/bin/vidpic

