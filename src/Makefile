prefix=/usr
includedir=$(prefix)/include
libdir=$(prefix)/lib

CFLAGS ?= -g -fomit-frame-pointer -O2
CFLAGS += -nostdlib -nostartfiles -Wall -I. -fPIC
SO_CFLAGS=-shared $(CFLAGS)
L_CFLAGS=$(CFLAGS)
LINK_FLAGS=
LINK_FLAGS+=$(LDFLAGS)

DEBUG_FLAGS=-DLNVM_DEBUG_ENABLED

soname=liblightnvm.so.1
minor=0
micro=0
libname=$(soname).$(minor).$(micro)
all_targets += liblightnvm.a $(libname)

all: $(all_targets)

# liblightnvm provided functions
liblightnvm_srcs := mgmt.c provisioning.c flash_io.c dflash.c

liblightnvm_objs := $(patsubst %.c,%.ol,$(liblightnvm_srcs))
liblightnvm_sobjs := $(patsubst %.c,%.os,$(liblightnvm_srcs))

$(liblightnvm_objs) $(liblightnvm_sobjs): liblightnvm.h

%.os: %.c
	$(CC) $(SO_CFLAGS) -c -o $@ $<

%.ol: %.c
	$(CC) $(L_CFLAGS) -c -o $@ $<

AR ?= ar
RANLIB ?= ranlib
liblightnvm.a: $(liblightnvm_objs)
	rm -f liblightnvm.a
	$(AR) r liblightnvm.a $^
	$(RANLIB) liblightnvm.a

$(libname): $(liblightnvm_sobjs) liblightnvm.map
	$(CC) $(SO_CFLAGS) -Wl,--version-script=liblightnvm.map -Wl,-soname=$(soname) -o $@ $(liblightnvm_sobjs) $(LINK_FLAGS)

install: $(all_targets)
	install -D -m 644 liblightnvm.h $(includedir)/liblightnvm.h
	install -D -m 644 liblightnvm.a $(libdir)/liblightnvm.a
	install -D -m 755 $(libname) $(libdir)/$(libname)
	ln -sf $(libname) $(libdir)/$(soname)
	ln -sf $(libname) $(libdir)/liblightnvm.so

install_local:
	install -D -m 644 ioctl.h $(includedir)/linux/lightnvm.h

debug: install
debug: SO_CFLAGS += $(DEBUG_FLAGS)

$(liblightnvm_objs): liblightnvm.h

clean:
	rm -f $(all_targets) $(liblightnvm_objs) $(liblightnvm_sobjs) $(soname).new
	rm -f *.so* *.a *.o
