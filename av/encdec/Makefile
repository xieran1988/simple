
include ../../top.mk

all: a.a test filter_test $T/bin/av-probe

a.a: dec.o enc.o
	$A $@ $^

test: dec.o enc.o test.o
	${CC} -o $@ $^ ${AVLIB} $T/av/util/a.a

$T/bin/av-probe: probe.o 
	${CC} -o $@ $^ ${AVLIB} $T/av/util/a.a

filter_test: filter.o
	${CC} -o $@ $^ ${AVLIB} $T/av/util/a.a

clean: OBJ += $T/bin/av-probe
