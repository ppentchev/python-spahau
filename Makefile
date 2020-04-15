#!/usr/bin/make -f

PROG_C=	c/spahau

all:
	printf '\n\n===== %s\n' 'Building {PROG_C}'
	${MAKE} -C c

test:	test-c

test-c:	${PROG_C}
	printf '\n\n===== %s\n' 'Running the self-test on ${PROG_C}'
	${MAKE} -C c test
	printf '\n\n===== %s\n' 'Running the test suite on ${PROG_C}'
	env TEST_PROG='${PROG_C}' prove -v t

clean:	clean-c

clean-c:
	${MAKE} -C c clean

.PHONY:	all test test-c clean clean-c
