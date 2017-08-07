ROOT=
SUPERUSER=root
SUPERGROUP=root

VERSION := $(shell awk '/Version:/ { print $$2 }' initscripts.spec)
TAG=$(VERSION)

mandir=/usr/share/man

all:
	make -C src
	make -C po

install:
	mkdir -p $(ROOT)/etc/profile.d $(ROOT)/usr/sbin
	mkdir -p $(ROOT)$(mandir)/man{5,8}
	mkdir -p $(ROOT)/etc/rwtab.d $(ROOT)/etc/statetab.d
	mkdir -p $(ROOT)/var/lib/stateless/writable
	mkdir -p $(ROOT)/var/lib/stateless/state

	install -m644  adjtime $(ROOT)/etc
	install -m644  rwtab statetab networks $(ROOT)/etc
	install -m755  service $(ROOT)/usr/sbin
	install -m644  lang.csh lang.sh $(ROOT)/etc/profile.d
	install -m644  debug.csh debug.sh $(ROOT)/etc/profile.d
	install -m755  sys-unconfig $(ROOT)/usr/sbin
	install -m644  service.8 sys-unconfig.8 $(ROOT)$(mandir)/man8
	if uname -m | grep -q sparc ; then \
	  install -D -m644 sysctl.conf.sparc $(ROOT)/usr/lib/sysctl.d/00-system.conf ; fi
	if uname -m | grep -q s390 ; then \
	  install -D -m644 sysctl.conf.s390 $(ROOT)/usr/lib/sysctl.d/00-system.conf ; fi

	install -m755 -d $(ROOT)/etc/rc.d $(ROOT)/etc/sysconfig
	cp -af rc.d/init.d $(ROOT)/etc/rc.d/
	install -m644 sysconfig/debug sysconfig/init sysconfig/netconsole sysconfig/readonly-root $(ROOT)/etc/sysconfig/
	cp -af sysconfig/network-scripts $(ROOT)/etc/sysconfig/
	cp -af NetworkManager $(ROOT)/etc
	mkdir -p $(ROOT)/usr/lib/systemd/
	cp -af systemd/* $(ROOT)/usr/lib/systemd/
	mkdir -p $(ROOT)/usr/lib
	cp -af udev $(ROOT)/usr/lib
	chmod 755 $(ROOT)/etc/rc.d/* $(ROOT)/etc/rc.d/init.d/*
	chmod 644 $(ROOT)/etc/rc.d/init.d/functions
	chmod 755 $(ROOT)/etc/sysconfig/network-scripts/ifup-*
	chmod 755 $(ROOT)/etc/sysconfig/network-scripts/ifdown-*
	chmod 755 $(ROOT)/etc/sysconfig/network-scripts/init*
	chmod 755 $(ROOT)/etc/NetworkManager/dispatcher.d/00-netreport
	mkdir -p $(ROOT)/etc/sysconfig/modules
	mkdir -p $(ROOT)/etc/sysconfig/console
	if uname -m | grep -q s390 ; then \
	  install -m644 sysconfig/init.s390 $(ROOT)/etc/sysconfig/init ; \
	fi

	mv $(ROOT)/etc/sysconfig/network-scripts/ifup $(ROOT)/usr/sbin
	mv $(ROOT)/etc/sysconfig/network-scripts/ifdown $(ROOT)/usr/sbin
	(cd $(ROOT)/etc/sysconfig/network-scripts; \
	  ln -sf ifup-ippp ifup-isdn ; \
	  ln -sf ifdown-ippp ifdown-isdn ; \
	  ln -sf ../../../usr/sbin/ifup . ; \
	  ln -sf ../../../usr/sbin/ifdown . )
	make install ROOT=$(ROOT) mandir=$(mandir) -C src
	make install PREFIX=$(ROOT) -C po

	mkdir -p $(ROOT)/var/log
	# do not touch /run if installing into a chroot
	if -z "$(ROOT)"; then \
	  mkdir -p /run/netreport; \
	  chown $(SUPERUSER):$(SUPERGROUP) /run/netreport; \
	  chmod u=rwx,g=rwx,o=rx /run/netreport; \
	fi

	for i in 0 1 2 3 4 5 6 ; do \
		dir=$(ROOT)/etc/rc.d/rc$$i.d; \
	  	mkdir $$dir; \
		chmod u=rwx,g=rx,o=rx $$dir; \
	done

# Can't store symlinks in a CVS archive
	mkdir -p -m 755 $(ROOT)/usr/lib/systemd/system/multi-user.target.wants
	mkdir -p -m 755 $(ROOT)/usr/lib/systemd/system/graphical.target.wants
	mkdir -p -m 755 $(ROOT)/usr/lib/systemd/system/local-fs.target.wants
	mkdir -p -m 755 $(ROOT)/usr/lib/systemd/system/basic.target.wants
	mkdir -p -m 755 $(ROOT)/usr/lib/systemd/system/sysinit.target.wants
	ln -s ../fedora-loadmodules.service $(ROOT)/usr/lib/systemd/system/basic.target.wants
	ln -s ../fedora-readonly.service $(ROOT)/usr/lib/systemd/system/local-fs.target.wants
	ln -s ../fedora-import-state.service $(ROOT)/usr/lib/systemd/system/local-fs.target.wants

	mkdir -p $(ROOT)/usr/lib/tmpfiles.d
	install -m 644 initscripts.tmpfiles.d $(ROOT)/usr/lib/tmpfiles.d/initscripts.conf

# These are LSB compatibility symlinks.  At some point in the future
# the actual files will be here instead of symlinks
	for i in 0 1 2 3 4 5 6 ; do \
		ln -s rc.d/rc$$i.d $(ROOT)/etc/rc$$i.d; \
	done

	mkdir -p -m 755 $(ROOT)/usr/libexec/initscripts/legacy-actions


syntax-check:
	for afile in `find . -type f -perm +111|grep -v \.csh | grep -v .git | grep -v po/ ` ; do \
		if ! file $$afile | grep -s ELF  >/dev/null; then \
		    bash -n $$afile || { echo $$afile ; exit 1 ; } ; \
		fi  ;\
	done

check: syntax-check
	make check -C src
	make clean -C src

changelog:
	@rm -f ChangeLog
	git log --stat > ChangeLog

clean:
	make clean -C src
	make clean -C po
	@rm -fv *~ changenew ChangeLog.old *gz
	@find . -name "*~" -exec rm -v -f {} \;

tag:
	@git tag -a -f -m "Tag as $(TAG)" $(TAG)
	@echo "Tagged as $(TAG)"
