ROOT=/

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

clean:
	(cd src; make clean)
