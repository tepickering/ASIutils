CC = gcc
CFLAGS = -g -IASI_SDK/include
LDFLAGS = -lm -lcfitsio -lxpa -LASI_SDK/lib/mac -lASICamera2
EXECS = test_cap

all:	${EXECS}

clean:
	rm -f *.o; rm -rf *~; rm -f ${EXECS}

test_cap:	test_cap.c
	${CC} ${CFLAGS} test_cap.c ${LDFLAGS} -o test_cap
