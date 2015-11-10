IDIR=CuTest
CC=gcc
CFLAGS=-I.

DEPS = CuTest/CuTest.h
OBJ = dflash_tests.o CuTest/CuTest.o
LIBOBJ = lib_test.o CuTest/CuTest.o

all: dflash_test lib_test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

dflash_test: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) -llightnvm -lpthread

lib_test: $(LIBOBJ)
	gcc -o $@ $^ $(CFLAGS) -llightnvm -lpthread

clean:
	rm -f *.o dflash_test lib_test

check:
	dflash_test lib_test
