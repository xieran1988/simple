
include ../../top.mk

all: test objc_test a.a

LIB := ${GLLIB} ${AVLIB} ${FTLIB} ${JPGLIB}
LIB += $T/av/encdec/a.a 
LIB += $T/av/util/a.a 
LIB += $T/gl/util/util.o 

a.a: shader.o yuvtex.o fontex.o rgbtex.o fbotex.o utils.o a.o
	$A $@ $^ 

test: test.o a.a
	${CC} -o $@ $^ ${LIB}

objc_test: objc.o a.a
	${CC} -o $@ $^ ${LIB}

rtmp:
	ffplay rtmp://localhost/myapp/1

disp-y:
	display -depth 8 -size 640x360 /tmp/Y.gray
	
disp-u:
	display -depth 8 -size 64x64 /tmp/U.gray
	
disp-v:
	display -depth 8 -size 64x64 /tmp/V.gray
	
