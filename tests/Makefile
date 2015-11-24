IDIR=CuTest
CC=gcc
CFLAGS=-I.

DEPS = CuTest/CuTest.h
OBJ = append_tests.o CuTest/CuTest.o
LIBOBJ = lib_test.o CuTest/CuTest.o

all: append_test lib_test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

append_test: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) -llightnvm -lpthread

lib_test: $(LIBOBJ)
	gcc -o $@ $^ $(CFLAGS) -llightnvm -lpthread

check: append_test lib_test
	sh runtests.sh

clean:
	rm -f *.o append_test lib_test
	rm -rf CuTest/*.o
