CFLAGS+=$(RPM_OPT_FLAGS) -Wall -D_GNU_SOURCE -fPIE
LDFLAGS+=$(RPM_LD_FLAGS) -pie -z relro -z now

PROGS=usernetctl netreport usleep ipcalc brandbot \
	ppp-watch consoletype genhostid rename_device udev-kvm-check
PPPWATCH_OBJS=ppp-watch.o shvar.o
BRANDBOT_OBJS=brandbot.o shvar.o
CONSOLE_INIT_OBJS=console_init.o shvar.o
USLEEP_OBJS=usleep.o
UDEV_KVM_CHECK_OBJS=udev-kvm-check.o shvar.o

mandir=/usr/share/man

all:	$(PROGS)

check:	all
	./ipcalc-tests

clean:
	rm -f $(PROGS) *.o *~

install:
	mkdir -p $(ROOT)/usr/bin $(ROOT)/usr/sbin $(ROOT)$(mandir)/man{1,8} $(ROOT)/etc $(ROOT)/usr/lib/udev $(ROOT)/usr/libexec/initscripts/
	install -m 755 usleep $(ROOT)/usr/bin/usleep
	install -m 4755 usernetctl $(ROOT)/usr/sbin/usernetctl
	install -m 2755 netreport $(ROOT)/usr/sbin/netreport
	install -m 755 ipcalc $(ROOT)/usr/bin/ipcalc
	install -m 755 genhostid $(ROOT)/usr/sbin/genhostid
	install -m 755 ppp-watch $(ROOT)/usr/sbin/ppp-watch
	install -m 755 consoletype $(ROOT)/usr/sbin/consoletype
	install -m 755 sushell $(ROOT)/usr/sbin/sushell
	install -m 755 brandbot $(ROOT)/usr/libexec/initscripts/brandbot
	install -m 755 rename_device $(ROOT)/usr/lib/udev/rename_device
	install -m 755 udev-kvm-check $(ROOT)/usr/lib/udev/udev-kvm-check
	install -m 644 genhostid.1 $(ROOT)$(mandir)/man1
	install -m 644 netreport.1 $(ROOT)$(mandir)/man1
	install -m 644 usleep.1 $(ROOT)$(mandir)/man1
	install -m 644 usernetctl.8 $(ROOT)$(mandir)/man8
	install -m 644 ppp-watch.8 $(ROOT)$(mandir)/man8
	install -m 644 ipcalc.1 $(ROOT)$(mandir)/man1
	install -m 644 consoletype.1 $(ROOT)$(mandir)/man1
	install -m 644 ifup.8 $(ROOT)$(mandir)/man8
	install -m 644 sushell.8 $(ROOT)$(mandir)/man8
	ln -s ifup.8 $(ROOT)$(mandir)/man8/ifdown.8

ipcalc: ipcalc.o
	$(CC) -D_GNU_SOURCE $(LDFLAGS) -o $@ $< -lpopt

usleep: $(USLEEP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(USLEEP_OBJS) -lpopt

ppp-watch: $(PPPWATCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(PPPWATCH_OBJS) `pkg-config glib-2.0 --libs`

brandbot: $(BRANDBOT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(BRANDBOT_OBJS) `pkg-config glib-2.0 --libs`

udev-kvm-check: $(UDEV_KVM_CHECK_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(UDEV_KVM_CHECK_OBJS) `pkg-config glib-2.0 --libs`

usernetctl.o: usernetctl.c
	$(CC) $(CFLAGS) -c usernetctl.c -o usernetctl.o

usernetctl: usernetctl.c usernetctl.o
	$(CC) $(LDFLAGS) -o $@ $@.o

netreport.o: netreport.c
	$(CC) $(CFLAGS) -c netreport.c -o netreport.o

netreport: netreport.o
	$(CC) $(LDFLAGS) -o $@ $@.o

shvar.o: shvar.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c shvar.c -o shvar.o

brandbot.o: brandbot.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c brandbot.c -o brandbot.o

ppp-watch.o: ppp-watch.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c ppp-watch.c -o ppp-watch.o

rename_device: rename_device.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -o $@ $< $(LDFLAGS) `pkg-config glib-2.0 --libs`

udev-kvm-check.o: udev-kvm-check.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c udev-kvm-check.c -o udev-kvm-check.o

