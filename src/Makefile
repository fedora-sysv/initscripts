# Basic Makefile for compiling & installing the files.
#
# Supports standard GNU Makefile variables for specifying the paths:
# * prefix
# * bindir
# * sbindir
# * libdir
# * DESTDIR
#

SHELL       = /bin/bash

# Normally /usr/local is used. However, it does not make sense for us to use it
# here, as it just complicates things even further.
prefix      = /usr
bindir      = $(prefix)/bin
sbindir     = $(prefix)/sbin
libdir      = $(prefix)/lib

CC          = gcc
CFLAGS     += $(RPM_OPT_FLAGS) -Wall -D_GNU_SOURCE -fPIE
LDFLAGS    += $(RPM_LD_FLAGS) -pie -z relro -z now

PROGS          = consoletype genhostid rename_device usleep
INSTALL_PROGS  = install-progs
ifndef NO_NETWORK_SCRIPTS
PROGS         += usernetctl
INSTALL_PROGS += install-network-scripts
endif

all: $(PROGS)

install: $(INSTALL_PROGS)

install-progs:
	install -m 0755 -d $(DESTDIR)$(bindir)
	install -m 0755 -d $(DESTDIR)$(sbindir)
	install -m 0755 -d $(DESTDIR)$(libdir)/udev
	install -m 0755 build/usleep         $(DESTDIR)$(bindir)
	install -m 0755 build/consoletype    $(DESTDIR)$(sbindir)
	install -m 0755 build/genhostid      $(DESTDIR)$(sbindir)
	install -m 0755 build/rename_device  $(DESTDIR)$(libdir)/udev

install-network-scripts:
	install -m 0755 build/usernetctl     $(DESTDIR)$(sbindir)

clean:
	rm -f build/*

consoletype: build/consoletype.o
	$(CC) $(LDFLAGS) -o build/$@ $^

build/consoletype.o: consoletype.c
	$(CC) $(CFLAGS) -c -o $@ $^


genhostid: build/genhostid.o
	$(CC) $(LDFLAGS) -o build/$@ $^

build/genhostid.o: genhostid.c
	$(CC) $(CFLAGS) -c -o $@ $^


rename_device: build/rename_device.o
	$(CC) $(LDFLAGS) -o build/$@ $^ `pkg-config glib-2.0 --libs`

build/rename_device.o: rename_device.c
	$(CC) $(CFLAGS) -c -o $@ $^ `pkg-config glib-2.0 --cflags`


usernetctl: build/usernetctl.o
	$(CC) $(LDFLAGS) -o build/$@ $^

build/usernetctl.o: usernetctl.c
	$(CC) $(CFLAGS) -c -o $@ $^


usleep: build/usleep.o
	$(CC) $(LDFLAGS) -o build/$@ $^ -lpopt

build/usleep.o: usleep.c
	$(CC) $(CFLAGS) -c -o $@ $^
