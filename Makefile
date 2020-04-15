#!/usr/bin/make -f

all:
	${MAKE} -C c

test:	all
	${MAKE} -C c test

clean:
	${MAKE} -C c clean

.PHONY:	all test clean
