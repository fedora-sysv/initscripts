ROOT=/
SUPERUSER=root
SUPERGROUP=root

VERSION=$(shell awk '/Version:/ { print $$2 }' initscripts.spec)
CVSTAG = r$(subst .,-,$(VERSION))
CVSROOT = $(shell cat CVS/Root)


mandir=/usr/share/man

all:
	make -C src
	make -C po

install:
	mkdir -p $(ROOT)/etc/profile.d $(ROOT)/sbin $(ROOT)/usr/sbin
	mkdir -p $(ROOT)$(mandir)/man{5,8}
	mkdir -p $(ROOT)/etc/rwtab.d $(ROOT)/etc/statetab.d
	mkdir -p $(ROOT)/var/lib/stateless/writable
	mkdir -p $(ROOT)/var/lib/stateless/state

	install -m644  inittab adjtime $(ROOT)/etc
	if uname -m | grep -q s390 ; then \
	  install -m644 inittab.s390 $(ROOT)/etc/inittab ; \
	fi
	install -m644  rwtab statetab networks $(ROOT)/etc
	install -m755  service setsysfont $(ROOT)/sbin
	install -m644  lang.csh lang.sh $(ROOT)/etc/profile.d
	install -m755  sys-unconfig $(ROOT)/usr/sbin
	install -m644  crypttab.5 $(ROOT)$(mandir)/man5
	install -m644  service.8 sys-unconfig.8 $(ROOT)$(mandir)/man8
	install -m644 sysctl.conf $(ROOT)/etc/sysctl.conf
	if uname -m | grep -q sparc ; then \
	  install -m644 sysctl.conf.sparc $(ROOT)/etc/sysctl.conf ; fi
	if uname -m | grep -q s390 ; then \
	  install -m644 sysctl.conf.s390 $(ROOT)/etc/sysctl.conf ; fi

	mkdir -p $(ROOT)/etc/X11
	install -m755 prefdm $(ROOT)/etc/X11/prefdm

	install -m755 -d $(ROOT)/etc/rc.d $(ROOT)/etc/sysconfig
	install -m755 rc.d/rc rc.d/rc.local rc.d/rc.sysinit $(ROOT)/etc/rc.d/
	cp -af rc.d/init.d $(ROOT)/etc/rc.d/
	install -m755 sysconfig/init sysconfig/netconsole sysconfig/readonly-root $(ROOT)/etc/sysconfig/
	cp -af sysconfig/network-scripts $(ROOT)/etc/sysconfig/
	cp -af ppp udev NetworkManager $(ROOT)/etc
	mkdir -p $(ROOT)/etc/ppp/peers
	chmod 755 $(ROOT)/etc/rc.d/* $(ROOT)/etc/rc.d/init.d/*
	chmod 755 $(ROOT)/etc/ppp/peers
	chmod 755 $(ROOT)/etc/ppp/ip*
	chmod 755 $(ROOT)/etc/sysconfig/network-scripts/ifup-*
	chmod 755 $(ROOT)/etc/sysconfig/network-scripts/ifdown-*
	chmod 755 $(ROOT)/etc/sysconfig/network-scripts/init*
	chmod 755 $(ROOT)/etc/sysconfig/network-scripts/net.hotplug
	chmod 755 $(ROOT)/etc/NetworkManager/dispatcher.d/00-netreport
	mkdir -p $(ROOT)/etc/sysconfig/modules
	mkdir -p $(ROOT)/etc/sysconfig/networking/devices
	mkdir -p $(ROOT)/etc/sysconfig/networking/profiles/default
	#mv  $(ROOT)/etc/sysconfig/network-scripts/ifcfg-lo \
	#	$(ROOT)/etc/sysconfig/networking/devices
	#ln -s ../networking/devices/ifcfg-lo \
	#   	$(ROOT)/etc/sysconfig/network-scripts/ifcfg-lo
	#ln -s ../networking/devices/ifcfg-lo \
	#	$(ROOT)/etc/sysconfig/networking/profiles/default/ifcfg-lo
	mkdir -p $(ROOT)/etc/sysconfig/console
	if uname -m | grep -q s390 ; then \
	  install -m644 sysconfig/init.s390 $(ROOT)/etc/sysconfig/init ; \
	fi

	mv $(ROOT)/etc/sysconfig/network-scripts/ifup $(ROOT)/sbin
	mv $(ROOT)/etc/sysconfig/network-scripts/ifdown $(ROOT)/sbin
	(cd $(ROOT)/etc/sysconfig/network-scripts; \
	  ln -sf ifup-ippp ifup-isdn ; \
	  ln -sf ifdown-ippp ifdown-isdn ; \
	  ln -sf ../../../sbin/ifup . ; \
	  ln -sf ../../../sbin/ifdown . )
	make install ROOT=$(ROOT) mandir=$(mandir) -C src
	make install PREFIX=$(ROOT) -C po

	mkdir -p $(ROOT)/var/run/netreport $(ROOT)/var/log
	chown $(SUPERUSER):$(SUPERGROUP) $(ROOT)/var/run/netreport
	chmod u=rwx,g=rwx,o=rx $(ROOT)/var/run/netreport
	touch $(ROOT)/var/run/utmp
	touch $(ROOT)/var/log/wtmp
	touch $(ROOT)/var/log/btmp

	for i in 0 1 2 3 4 5 6 ; do \
		dir=$(ROOT)/etc/rc.d/rc$$i.d; \
	  	mkdir $$dir; \
		chmod u=rwx,g=rx,o=rx $$dir; \
	done

# Can't store symlinks in a CVS archive
	ln -s ../init.d/killall $(ROOT)/etc/rc.d/rc0.d/S00killall
	ln -s ../init.d/killall $(ROOT)/etc/rc.d/rc6.d/S00killall

	ln -s ../init.d/halt $(ROOT)/etc/rc.d/rc0.d/S01halt
	ln -s ../init.d/halt $(ROOT)/etc/rc.d/rc6.d/S01reboot

	ln -s ../init.d/single $(ROOT)/etc/rc.d/rc1.d/S99single

	ln -s ../rc.local $(ROOT)/etc/rc.d/rc2.d/S99local
	ln -s ../rc.local $(ROOT)/etc/rc.d/rc3.d/S99local
	ln -s ../rc.local $(ROOT)/etc/rc.d/rc4.d/S99local
	ln -s ../rc.local $(ROOT)/etc/rc.d/rc5.d/S99local

# These are LSB compatibility symlinks.  At some point in the future
# the actual files will be here instead of symlinks
	for i in 0 1 2 3 4 5 6 ; do \
		ln -s rc.d/rc$$i.d $(ROOT)/etc/rc$$i.d; \
	done
	for i in rc rc.sysinit rc.local ; do \
		ln -s rc.d/$$i $(ROOT)/etc/$$i; \
	done



check:
	for afile in `find . -type f -perm +111|grep -v \.csh | grep -v po/ ` ; do \
		if ! file $$afile | grep -s ELF  >/dev/null; then \
		    bash -n $$afile || { echo $$afile ; exit 1 ; } ; \
		fi  ;\
	done

changelog:
	rcs2log -h redhat.com -r -b -r "-d>`awk '{ print $$1 ; exit }' ChangeLog `" > changenew
	 mv ChangeLog ChangeLog.old
	 cat changenew ChangeLog.old > ChangeLog
	 rm -f changenew

clean:
	make clean -C src
	make clean -C po
	@rm -fv *~ changenew ChangeLog.old *gz
tag-archive:
	@cvs -Q tag -F $(CVSTAG)

create-archive: tag-archive
	@rm -rf /tmp/initscripts
	@cd /tmp; cvs -Q -d $(CVSROOT) export -r$(CVSTAG) initscripts || echo GRRRrrrrr -- ignore [export aborted]
	@mv /tmp/initscripts /tmp/initscripts-$(VERSION)
	@cd /tmp; tar --bzip2 -cSpf initscripts-$(VERSION).tar.bz2 initscripts-$(VERSION)
	@rm -rf /tmp/initscripts-$(VERSION)
	@cp /tmp/initscripts-$(VERSION).tar.bz2 .
	@rm -f /tmp/initscripts-$(VERSION).tar.bz2 
	@echo " "
	@echo "The final archive is ./initscripts-$(VERSION).tar.bz2."

archive: clean check tag-archive create-archive
