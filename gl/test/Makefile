
include ../../top.mk

all: old

LIB := ${GLLIB} ${AVLIB} ${FTLIB} ${JPGLIB}
LIB += $T/av/encdec/a.a 
LIB += $T/av/util/a.a 
#LIB += $T/gl/vid/vid.o 
#LIB += $T/gl/shader/shader.o 
LIB += $T/gl/util/util.o 

old: old.o shader.o yuvtex.o fontex.o
	${CC} -o $@ $^ ${LIB}


