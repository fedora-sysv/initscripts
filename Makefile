ROOT=/

VERSION=$(shell awk '/define version/ { print $$3 }' initscripts.spec)
CVSTAG = r$(subst .,-,$(VERSION))

all:
	(cd src; make CFLAGS="$(CFLAGS)")

install:
	mkdir -p $(ROOT)/etc
	install -m644 -o root -g root inittab $(ROOT)/etc
	install -m644 -o root -g root adjtime $(ROOT)/etc
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
	@cvs tag -F $(CVSTAG)

create-archive: tag-archive
	@rm -rf /tmp/initscripts
	@cd /tmp/initscripts; cvs export -r$(CVSTAG) initscripts
	@mv /tmp/initscripts /tmp/initscripts-$(VERSION)
	@cd /tmp; tar czSpf initscripts-$(VERSION).tar.gz initscripts-$(VERSION)
	@rm -rf /tmp/initscripts-$(VERSION)
	@cp /tmp/initscripts-$(VERSION).tar.gz .
	@rm -f /tmp/initscripts-$(VERSION).tar.gz 
	@echo " "
	@echo "The final archive is ./initscripts-$(VERSION).tar.gz."

archive: tag-archive create-archive
