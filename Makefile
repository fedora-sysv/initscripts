ROOT=/

VERSION=$(shell awk '/define version/ { print $$3 }' initscripts.spec)
CVSTAG = r$(subst .,-,$(VERSION))

all:
	(cd src; make CFLAGS="$(CFLAGS)")

install:
	mkdir -p $(ROOT)/etc/profile.d $(ROOT)/sbin
	install -m644  inittab $(ROOT)/etc
	install -m644  adjtime $(ROOT)/etc
	install -m644  inputrc $(ROOT)/etc
	install -m755  setsysfont $(ROOT)/sbin
	install -m755  lang.sh $(ROOT)/etc/profile.d
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

archive: tag-archive create-archive
