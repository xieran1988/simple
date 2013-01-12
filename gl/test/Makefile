
include ../../top.mk

all: old

LIB := ${GLLIB} ${AVLIB} ${FTLIB} ${JPGLIB}
LIB += $T/av/encdec/a.a 
LIB += $T/av/util/a.a 
#LIB += $T/gl/vid/vid.o 
#LIB += $T/gl/shader/shader.o 
LIB += $T/gl/util/util.o 

old: old.o shader.o yuvtex.o fontex.o rgbtex.o fbotex.o
	${CC} -o $@ $^ ${LIB}

disp-y:
	display -depth 8 -size 640x360 /tmp/Y.gray
	
disp-u:
	display -depth 8 -size 64x64 /tmp/U.gray
	
disp-v:
	display -depth 8 -size 64x64 /tmp/V.gray
	
