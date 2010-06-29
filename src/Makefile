CFLAGS+=$(RPM_OPT_FLAGS) -Wall -D_GNU_SOURCE

PROGS=usernetctl netreport testd usleep ipcalc \
	fstab-decode getkey ppp-watch consoletype genhostid rename_device \
	console_init console_check securetty
PPPWATCH_OBJS=ppp-watch.o shvar.o
CONSOLE_INIT_OBJS=console_init.o shvar.o
USLEEP_OBJS=usleep.o

mandir=/usr/share/man

all:	$(PROGS)

check:	all
	./ipcalc-tests

clean:
	rm -f $(PROGS) *.o *~

install:
	mkdir -p $(ROOT)/bin $(ROOT)/usr/sbin $(ROOT)$(mandir)/man{1,8} $(ROOT)/etc $(ROOT)/lib/udev
	install -m 755 usleep $(ROOT)/bin/usleep
	install -m 4755 usernetctl $(ROOT)/usr/sbin/usernetctl
	install -m 2755 netreport $(ROOT)/sbin/netreport
	install -m 755 ipcalc $(ROOT)/bin/ipcalc
	install -m 755 fstab-decode $(ROOT)/sbin/fstab-decode
	install -m 755 genhostid $(ROOT)/sbin/genhostid
	install -m 755 getkey $(ROOT)/sbin/getkey
	install -m 755 ppp-watch $(ROOT)/sbin/ppp-watch
	install -m 755 consoletype $(ROOT)/sbin/consoletype
	install -m 755 securetty $(ROOT)/sbin/securetty
	install -m 755 sushell $(ROOT)/sbin/sushell
	install -m 755 rename_device $(ROOT)/lib/udev/rename_device
	install -m 755 console_init $(ROOT)/lib/udev/console_init
	install -m 755 console_check $(ROOT)/lib/udev/console_check
	install -m 644 genhostid.1 $(ROOT)$(mandir)/man1
	install -m 644 getkey.1 $(ROOT)$(mandir)/man1
	install -m 644 netreport.1 $(ROOT)$(mandir)/man1
	install -m 644 usleep.1 $(ROOT)$(mandir)/man1
	install -m 644 fstab-decode.8 $(ROOT)$(mandir)/man8
	install -m 644 usernetctl.8 $(ROOT)$(mandir)/man8
	install -m 644 ppp-watch.8 $(ROOT)$(mandir)/man8
	install -m 644 ipcalc.1 $(ROOT)$(mandir)/man1
	install -m 644 consoletype.1 $(ROOT)$(mandir)/man1
	install -m 644 ifup.8 $(ROOT)$(mandir)/man8
	install -m 644 setsysfont.8 $(ROOT)$(mandir)/man8
	install -m 644 securetty.8 $(ROOT)$(mandir)/man8
	install -m 644 sushell.8 $(ROOT)$(mandir)/man8
	ln -s ifup.8 $(ROOT)$(mandir)/man8/ifdown.8

# this daemon and initscript are useful for testing the up/down/status stuff
# not installed by default, only comes from sources.
install-test:
	install -m 755 testd $(ROOT)/usr/sbin/testd
	install -m 755 testdinit $(ROOT)/etc/init.d/testd

getkey: getkey.o
	$(CC) $(LDFLAGS) -o $@ $< -lpopt

ipcalc: ipcalc.o
	$(CC) -D_GNU_SOURCE $(LDFLAGS) -o $@ $< -lpopt

usleep: $(USLEEP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(USLEEP_OBJS) -lpopt

ppp-watch: $(PPPWATCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(PPPWATCH_OBJS) `pkg-config glib-2.0 --libs`

usernetctl.o: usernetctl.c
	$(CC) $(CFLAGS) -fPIE -c usernetctl.c -o usernetctl.o

usernetctl: usernetctl.c usernetctl.o
	$(CC) $(LDFLAGS) -pie -o $@ $@.o

securetty: securetty.o
	$(CC) $(LDFLAGS) -o $@ $<

shvar.o: shvar.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c shvar.c -o shvar.o

ppp-watch.o: ppp-watch.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c ppp-watch.c -o ppp-watch.o

rename_device: rename_device.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -o $@ $< `pkg-config glib-2.0 --libs`

console_init.o: console_init.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c console_init.c -o console_init.o

console_init: $(CONSOLE_INIT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CONSOLE_INIT_OBJS) `pkg-config glib-2.0 --libs`
