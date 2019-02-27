CC = gcc
CFLAGS = -g -I../include
LDFLAGS = -lm -lcfitsio -lxpa -L../lib/mac64 -lASICamera2
EXECS = test_cap

all:	${EXECS}

clean:
	rm -f *.o; rm -rf *~; rm -f ${EXECS}

test_cap:	test_cap.c
	${CC} ${CFLAGS} test_cap.c ${LDFLAGS} -o test_cap
