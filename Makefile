ROOT=/

VERSION=$(shell awk '/define version/ { print $$3 }' initscripts.spec)
CVSTAG = r$(subst .,-,$(VERSION))

all:
	(cd src; make CFLAGS="$(CFLAGS)")

install:
	mkdir -p $(ROOT)/etc/profile.d $(ROOT)/sbin $(ROOT)/usr/sbin
	mkdir -p $(ROOT)/usr/man/man8
	install -m644  inittab $(ROOT)/etc
	install -m644  adjtime $(ROOT)/etc
	install -m755  setsysfont $(ROOT)/sbin
	install -m755  lang.sh $(ROOT)/etc/profile.d
	install -m755  lang.csh $(ROOT)/etc/profile.d
	install -m755  service $(ROOT)/sbin
	install -m755  sys-unconfig $(ROOT)/usr/sbin
	install -m644  sys-unconfig.8 $(ROOT)/usr/man/man8
	( if uname -m | grep -q sparc ; then \
	  install -m644 sysctl.conf.sparc $(ROOT)/etc/sysctl.conf ; \
	  else \
	  install -m644 sysctl.conf $(ROOT)/etc/sysctl.conf ; \
	  fi )
	mkdir -p $(ROOT)/etc/X11
	install -m755 prefdm $(ROOT)/etc/X11/prefdm
	mkdir -p $(ROOT)/etc/sysconfig
	mkdir -p $(ROOT)/etc/sysconfig/console
	install -m644 sysconfig/init $(ROOT)/etc/sysconfig/init
	cp -af rc.d sysconfig ppp $(ROOT)/etc
	mkdir -p $(ROOT)/sbin
	mv $(ROOT)/etc/sysconfig/network-scripts/ifup $(ROOT)/sbin
	mv $(ROOT)/etc/sysconfig/network-scripts/ifdown $(ROOT)/sbin
	(cd $(ROOT)/etc/sysconfig/network-scripts; \
	  ln -sf ../../../sbin/ifup . ; \
	  ln -sf ../../../sbin/ifdown . )
	(cd src; make install ROOT=$(ROOT))
	mkdir -p /var/run/netreport
	chown root.root /var/run/netreport
	chmod og=rwx,o=rx /var/run/netreport

check:
	for afile in `find . -type f -perm +111|grep -v \.csh ` ; do \
		if ! file $$afile | grep -s ELF  >/dev/null; then \
		    bash -n $$afile || { echo $$afile ; exit 1 } ; \
		fi  ;\
	done

changelog:
	rcs2log | sed "s|@.*redhat\.com|@redhat.com|" | sed "s|@@|@|" | \
	 sed "s|/mnt/devel/CVS/initscripts/||g" > changenew
	 mv ChangeLog ChangeLog.old
	 cat changenew ChangeLog.old > ChangeLog
	 rm -f changenew

clean:
	(cd src; make clean)
	@rm -fv *~ changenew ChangeLog.old

tag-archive:
	@cvs -Q tag -F $(CVSTAG)

create-archive: tag-archive
	@rm -rf /tmp/initscripts
	@cd /tmp; cvs -Q -d $(CVSROOT) export -r$(CVSTAG) initscripts || echo GRRRrrrrr -- ignore [export aborted]
	@mv /tmp/initscripts /tmp/initscripts-$(VERSION)
	@cd /tmp; tar czSpf initscripts-$(VERSION).tar.gz initscripts-$(VERSION)
	@rm -rf /tmp/initscripts-$(VERSION)
	@cp /tmp/initscripts-$(VERSION).tar.gz .
	@rm -f /tmp/initscripts-$(VERSION).tar.gz 
	@echo " "
	@echo "The final archive is ./initscripts-$(VERSION).tar.gz."

archive: clean check tag-archive create-archive
