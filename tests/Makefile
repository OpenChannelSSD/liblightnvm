IDIR=CuTest
CC=gcc
CFLAGS=-I.

DEPS = CuTest/CuTest.h
APPOBJ = append_tests.o CuTest/CuTest.o
RAWOBJ = raw_tests.o CuTest/CuTest.o
LIBOBJ = lib_test.o CuTest/CuTest.o

all: append_test raw_test lib_test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

append_test: $(APPOBJ)
	gcc -o $@ $^ $(CFLAGS) -llightnvm -lpthread

raw_test: $(RAWOBJ)
	gcc -o $@ $^ $(CFLAGS) -llightnvm -lpthread

lib_test: $(LIBOBJ)
	gcc -o $@ $^ $(CFLAGS) -llightnvm -lpthread

check: append_test raw_test lib_test
	sh runtests.sh

clean:
	rm -f *.o append_test raw_test lib_test
	rm -rf CuTest/*.o
