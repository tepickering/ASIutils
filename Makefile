CC = gcc
CFLAGS = -g -IASI_SDK/include -I/usr/local/include
LDFLAGS = -lm -lc -lcfitsio -lxpa -largp -LASI_SDK/lib/mac -lASICamera2
EXECS = test_cap

all:	${EXECS}

clean:
	rm -f *.o; rm -rf *~; rm -f ${EXECS}; rm -rf *.dSYM

test_cap:	test_cap.c
	${CC} ${CFLAGS} test_cap.c ${LDFLAGS} -o test_cap
	install_name_tool -change '@loader_path/libASICamera2.dylib' '@executable_path/ASI_SDK/lib/mac/libASICamera2.dylib' test_cap
