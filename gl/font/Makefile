
include ../../top.mk

all: font.so

font.so: font.o
	${SO} -o $@ $< ${GLLIB} ${FTLIB}

font.o: CFLAGS += `freetype-config --cflags`
