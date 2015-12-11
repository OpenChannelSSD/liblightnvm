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

debug:
	@$(MAKE) -C src debug prefix=$(prefix) includedir=$(includedir) libdir=$(libdir)
	@$(MAKE) -C tests
	ln -sf tests/lib_test .
	ln -sf tests/append_test .
	ln -sf tests/raw_test .

tests_check:
	@$(MAKE) -C tests check

tags:
	ctags * -R
	cscope -b `find . -name '*.c'` `find . -name '*.h'`

clean:
	@$(MAKE) -C src clean
	@$(MAKE) -C tests clean
	rm -rf lib_test append_test

check: clean install tests_check
