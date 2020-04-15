#!/usr/bin/make -f

PROG_C=		c/spahau
PROG_PY=	python/run_spahau.sh

all:	all-c all-python

all-c:
	printf '\n\n===== %s\n' 'Building {PROG_C}'
	${MAKE} -C c

all-python:
	printf '\n\n===== %s\n' 'Building a Python source tarball'
	(set -e; cd python; python3 setup.py sdist)

test:	test-c test-python

test-c:	all-c
	printf '\n\n===== %s\n' 'Running the self-test on ${PROG_C}'
	${MAKE} -C c test
	printf '\n\n===== %s\n' 'Running the test suite on ${PROG_C}'
	env TEST_PROG='${PROG_C}' prove -v t

test-python:
	printf '\n\n===== %s\n' 'Running the test suite on ${PROG_PY}'
	env TEST_PROG='${PROG_PY}' prove -v t

clean:	clean-c clean-python

clean-c:
	${MAKE} -C c clean

clean-python:
	rm -rf python/.mypy_cache python/.tox python/build python/dist
	find python/ -mindepth 1 -maxdepth 2 -type d -name '*.egg-info' -print0 | xargs -0r rm -rf
	find python/ -type f -name '*.pyc' -delete
	find python/ -type d -name '__pycache__' -print0 | xargs -0r rm -rf

.PHONY:	all all-c test test-c clean clean-c
