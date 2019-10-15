# Basic Makefile for compiling & installing the files.
#
# Supports standard GNU Makefile variables for specifying the paths:
# * prefix
# * exec_prefix
# * bindir
# * sbindir
# * libdir
# * datarootdir
# * datadir
# * mandir
# * sysconfdir
# * localstatedir
# * DESTDIR
#

SHELL          = /bin/bash

# Normally /usr/local is used. However, it does not make sense for us to use it
# here, as it just complicates things even further.
prefix         = /usr
exec_prefix    = $(prefix)
bindir         = $(prefix)/bin
sbindir        = $(prefix)/sbin
libdir         = $(prefix)/lib
libexecdir     = $(exec_prefix)/libexec
datarootdir    = $(prefix)/share
datadir        = $(datarootdir)
mandir         = $(datadir)/man
sysconfdir     = /etc
localstatedir  = /var
sharedstatedir = $(localstatedir)/lib

VERSION       := $(shell gawk '/Version:/ { print $$2 }' initscripts.spec)
NEXT_VERSION  := $(shell gawk '/Version:/ { print $$2 + 0.01}' initscripts.spec)


all: make-binaries make-translations


make-binaries:
	$(MAKE) -C src

make-translations:
	$(MAKE) -C po PYTHON=$(PYTHON)


# NOTE: We are no longer installing into /usr/sbin directory, because this is
#       just a symlink to /usr/bin, thanks to UsrMove change. Instead, we just
#       use virtual provides for /usr/sbin/<utility> in specfile (for backward
#       compatibility).
install: install-binaries install-translations install-etc install-usr install-network-scripts install-man install-post


install-binaries:
	$(MAKE) install -C src DESTDIR=$(DESTDIR) prefix=$(prefix) bindir=$(bindir) libdir=$(libdir)

install-translations:
	$(MAKE) install -C po  DESTDIR=$(DESTDIR) prefix=$(prefix) bindir=$(bindir) libdir=$(libdir) \
	                                          datarootdir=$(datarootdir) datadir=$(datadir) sysconfdir=$(sysconfdir)


# NOTE: We are removing auxiliary symlink at the beginning.
install-etc:
	rm -f etc/sysconfig/network-scripts
	install -m 0755 -d $(DESTDIR)$(sysconfdir)
	cp -a        etc/* $(DESTDIR)$(sysconfdir)/

install-usr:
	install -m 0755 -d $(DESTDIR)$(prefix)
	cp -a        usr/* $(DESTDIR)$(prefix)/

install-network-scripts: install-usr install-etc
	install -m 0755 -d      $(DESTDIR)$(sysconfdir)/sysconfig/network-scripts
	cp -a network-scripts/* $(DESTDIR)$(sysconfdir)/sysconfig/network-scripts/
	ln -srf $(DESTDIR)$(sysconfdir)/sysconfig/network-scripts/{ifup-ippp,ifup-isdn}
	ln -srf $(DESTDIR)$(sysconfdir)/sysconfig/network-scripts/{ifdown-ippp,ifdown-isdn}

install-man: install-usr
	install -m 0755 -d      $(DESTDIR)$(mandir)/man1
	install -m 0755 -d      $(DESTDIR)$(mandir)/man8
	install -m 0644 man/*.1 $(DESTDIR)$(mandir)/man1
	install -m 0644 man/*.8 $(DESTDIR)$(mandir)/man8

# Initscripts still ship some empty directories necessary for system to function
# correctly...
install-post: install-etc
	install -m 0755 -d $(DESTDIR)$(sysconfdir)/sysconfig/console
	install -m 0755 -d $(DESTDIR)$(sysconfdir)/sysconfig/modules
	install -m 0755 -d $(DESTDIR)$(sharedstatedir)/stateless/state
	install -m 0755 -d $(DESTDIR)$(sharedstatedir)/stateless/writable
	install -m 0755 -d $(DESTDIR)$(libexecdir)/initscripts/legacy-actions
	for idx in {0..6}; do \
	    dir=$(DESTDIR)$(sysconfdir)/rc.d/rc$$idx.d; \
	    install -m 0755 -d $$dir; \
	    ln -srf $(DESTDIR)$(sysconfdir)/rc.d/rc$$idx.d $(DESTDIR)$(sysconfdir)/; \
	done
	ln -srf $(DESTDIR)$(sysconfdir)/rc.d/init.d $(DESTDIR)$(sysconfdir)/init.d

clean:
	$(MAKE) clean -C src
	$(MAKE) clean -C po
	@find . -name "*~" -exec rm -v -f {} \;

tag:
	@git tag -a -f -m "$(VERSION) release" $(VERSION)
	@echo "Tagged as $(VERSION)"

release-commit:
	@git log --decorate=no --format="- %s" $(VERSION)..HEAD > .changelog.tmp
	@rpmdev-bumpspec -n $(NEXT_VERSION) -f .changelog.tmp initscripts.spec
	@rm -f .changelog.tmp
	@git add initscripts.spec
	@git commit --message="$(NEXT_VERSION)"
	@echo -e "\n       New release commit ($(NEXT_VERSION)) created:\n"
	@git show

archive: clean
	@git archive --format=tar --prefix=initscripts-$(VERSION)/ HEAD > initscripts-$(VERSION).tar
	@mkdir -p initscripts-$(VERSION)/
	@tar --append -f initscripts-$(VERSION).tar initscripts-$(VERSION)
	@gzip -f initscripts-$(VERSION).tar
	@rm -rf initscripts-$(VERSION)
	@echo "The archive is at initscripts-$(VERSION).tar.gz"
