CC = gcc
CFLAGS = -g -IASI_SDK/include -I/usr/local/include
LDFLAGS = -lm -lc -lcfitsio -lxpa -largp -LASI_SDK/lib/mac -lASICamera2
EXECS = test_cap grab_cube

all:	${EXECS}

clean:
	rm -f *.o; rm -rf *~; rm -f ${EXECS}; rm -rf *.dSYM

test_cap:	test_cap.c
	${CC} ${CFLAGS} test_cap.c ${LDFLAGS} -o test_cap
	install_name_tool -change '@loader_path/libASICamera2.dylib' '@executable_path/ASI_SDK/lib/mac/libASICamera2.dylib' test_cap

grab_cube:	grab_cube.c
	${CC} ${CFLAGS} grab_cube.c ${LDFLAGS} -o grab_cube
	install_name_tool -change '@loader_path/libASICamera2.dylib' '@executable_path/ASI_SDK/lib/mac/libASICamera2.dylib' grab_cube
