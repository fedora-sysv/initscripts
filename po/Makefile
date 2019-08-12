# Basic Makefile for compiling & installing the translation files.
#
# Supports standard GNU Makefile variables for specifying the paths:
# * prefix
# * sbindir
# * libexecdir
# * datarootdir
# * datadir
# * sysconfdir
# * DESTDIR
#

SHELL       = /bin/bash

# Normally /usr/local is used. However, it does not make sense for us to use it
# here, as it just complicates things even further.
prefix      = /usr
sbindir     = $(prefix)/sbin
libexecdir  = $(prefix)/libexec
datarootdir = $(prefix)/share
datadir     = $(datarootdir)
sysconfdir  = /etc

NLSDIR      = $(datadir)/locale
NLSPACKAGE  = initscripts

CATALOGS    = $(shell ls *.po)
FMTCATALOGS = $(patsubst %.po,%.mo,$(CATALOGS))

POTFILES    = $(shell ls ../network-scripts/* | grep -v ifcfg-) \
	..$(sbindir)/service \
	..$(libexecdir)/import-state \
	..$(libexecdir)/loadmodules \
	..$(libexecdir)/readonly-root \
	..$(sysconfdir)/rc.d/init.d/*\


all: $(NLSPACKAGE).pot $(FMTCATALOGS)

$(NLSPACKAGE).pot: $(POTFILES)
	if [ -z "$(PYTHON)" ]; then \
		./xgettext_sh.py $(POTFILES) > $(NLSPACKAGE).po; \
	else \
		$(PYTHON) xgettext_sh.py $(POTFILES) > $(NLSPACKAGE).po; \
	fi
	if cmp -s $(NLSPACKAGE).po $(NLSPACKAGE).pot; then \
	    rm -f $(NLSPACKAGE).po; \
	else \
	    mv $(NLSPACKAGE).po $(NLSPACKAGE).pot; \
	fi

%.mo: %.po
	msgfmt -o $@ $<

install: all
	install -m 0755 -d $(DESTDIR)/$(NLSDIR)
	for file in $(CATALOGS); do \
	    lang=`basename $$file .po`; \
	    mo_file=$$lang.mo; \
	    install -m 0755 -d $(DESTDIR)$(NLSDIR)/$$lang; \
	    install -m 0755 -d $(DESTDIR)$(NLSDIR)/$$lang/LC_MESSAGES; \
	    install -m 0644 $$mo_file $(DESTDIR)/$(NLSDIR)/$$lang/LC_MESSAGES/$(NLSPACKAGE).mo; \
	done

clean:
	rm -f *.mo *.pyc
