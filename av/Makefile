
include ../top.mk

all: av.a av_test

av.a: mp4dec.o mp4enc.o
	$A $@ $^

av_test: mp4dec.o mp4enc.o av_test.o
	${CC} -o $@ $^ ${AVLIB}


