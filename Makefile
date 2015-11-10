NAME=liblightnvm

prefix=/usr
includedir=$(prefix)/include
libdir=$(prefix)/lib

default: all

all:
	@$(MAKE) -C src

install:
	@$(MAKE) -C src install prefix=$(prefix) includedir=$(includedir) libdir=$(libdir)

install_local:
	@$(MAKE) -C src install_local prefix=$(prefix) includedir=$(includedir) libdir=$(libdir)

test:
	@$(MAKE) -C tests
	ln -sf tests/lib_test .
	ln -sf tests/dflash_test .

clean:
	@$(MAKE) -C src clean
	@$(MAKE) -C tests clean
	rm -rf lib_test dflash_test
