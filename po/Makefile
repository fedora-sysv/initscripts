INSTALL= /usr/bin/install -c
INSTALL_PROGRAM= ${INSTALL}
INSTALL_DATA= ${INSTALL} -m 644
INSTALLNLSDIR=/etc/locale

MSGMERGE = msgmerge

NLSPACKAGE = initscripts

CATALOGS = $(shell ls *.po)
FMTCATALOGS = $(patsubst %.po,%.mo,$(CATALOGS))

POTFILES = /etc/rc.d/init.d/* /etc/rc.d/rc.* /etc/sysconfig/network-scripts/*

all: $(NLSPACKAGE).pot $(FMTCATALOGS)

$(NLSPACKAGE).pot: $(POTFILES)
	@echo "Please don't run 'make initscripts.pot unless you have a *FULL*, current install."

	#./xgettext_sh.py $(POTFILES) > $(NLSPACKAGE).po
	#if cmp -s $(NLSPACKAGE).po $(NLSPACKAGE).pot; then \
	#    rm -f $(NLSPACKAGE).po; \
	#else \
	#    mv $(NLSPACKAGE).po $(NLSPACKAGE).pot; \
	#fi


update-po: Makefile
	$(MAKE) $(NLSPACKAGE).pot
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
		lang=`basename $$cat .po`; \
		if $(MSGMERGE) $$lang.po $(NLSPACKAGE).pot > $$lang.pot ; then \
			mv -f $$lang.pot $$lang.po ; \
			echo "$(MSGMERGE) of $$lang succeeded" ; \
		else \
			echo "$(MSGMERGE) of $$lang failed" ; \
			rm -f $$lang.pot ; \
		fi \
	done

refresh-po: Makefile
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
		lang=`echo $$cat | sed 's/.po//'`; \
		if $(MSGMERGE) $$lang.po $(NLSPACKAGE).pot > $$lang.pot ; then \
			mv -f $$lang.pot $$lang.po ; \
			echo "$(MSGMERGE) of $$lang succeeded" ; \
		else \
			echo "$(MSGMERGE) of $$lang failed" ; \
			rm -f $$lang.pot ; \
		fi \
	done

report:
	@for cat in $(CATALOGS); do \
		echo -n "$$cat: "; \
		msgfmt -v --statistics -o /dev/null $$cat; \
	done

clean:
	rm -f *mo $(NLSPACKAGE).pot *.pyc

distclean: clean
	rm -f .depend Makefile

depend:

install:	all
	mkdir -p $(PREFIX)/$(INSTALLNLSDIR)
	for n in $(CATALOGS); do \
	    l=`basename $$n .po`; \
	    mo=$$l.mo; \
	    $(INSTALL) -m 755 -d $(PREFIX)/$(INSTALLNLSDIR)/$$l; \
	    $(INSTALL) -m 755 -d $(PREFIX)/$(INSTALLNLSDIR)/$$l/LC_MESSAGES; \
	    $(INSTALL) -m 644 $$mo \
		$(PREFIX)/$(INSTALLNLSDIR)/$$l/LC_MESSAGES/$(NLSPACKAGE).mo; \
	done

%.mo: %.po
	msgfmt -o $@ $<
