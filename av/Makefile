
include ../top.mk

all: av.so av_test

av.so: mp4dec.o mp4enc.o
	${CC} -shared -fPIC -o $@ $^ ${AVLIB}

av_test: mp4dec.o mp4enc.o av_test.o
	${CC} -o $@ $^ ${AVLIB}


