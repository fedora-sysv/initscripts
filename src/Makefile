CFLAGS+=$(RPM_OPT_FLAGS) -Wall -D_GNU_SOURCE

PROGS=usernetctl doexec netreport testd usleep ipcalc initlog \
	getkey ppp-watch consoletype genhostid rename_device
PPPWATCH_OBJS=ppp-watch.o shvar.o
INITLOG_OBJS=initlog.o process.o
USLEEP_OBJS=usleep.o

mandir=/usr/share/man

all:	$(PROGS)

clean:
	rm -f $(PROGS) *.o

install:
	mkdir -p $(ROOT)/bin $(ROOT)/usr/sbin $(ROOT)$(mandir)/man{1,8} $(ROOT)/etc $(ROOT)/lib/udev
	install -m 755 doexec $(ROOT)/bin/doexec
	install -m 755 usleep $(ROOT)/bin/usleep
	install -m 4755 usernetctl $(ROOT)/usr/sbin/usernetctl
	install -m 2755 netreport $(ROOT)/sbin/netreport
	install -m 755 ipcalc $(ROOT)/bin/ipcalc
	install -m 755 genhostid $(ROOT)/sbin/genhostid
	install -m 755 initlog $(ROOT)/sbin/initlog
	install -m 755 getkey $(ROOT)/sbin/getkey
	install -m 755 ppp-watch $(ROOT)/sbin/ppp-watch
	install -m 755 consoletype $(ROOT)/sbin/consoletype
	install -m 755 rename_device $(ROOT)/lib/udev/rename_device
	install -m 644 initlog.1 $(ROOT)$(mandir)/man1
	install -m 644 genhostid.1 $(ROOT)$(mandir)/man1
	install -m 644 doexec.1 $(ROOT)$(mandir)/man1
	install -m 644 getkey.1 $(ROOT)$(mandir)/man1
	install -m 644 netreport.1 $(ROOT)$(mandir)/man1
	install -m 644 usleep.1 $(ROOT)$(mandir)/man1
	install -m 644 usernetctl.8 $(ROOT)$(mandir)/man8
	install -m 644 ppp-watch.8 $(ROOT)$(mandir)/man8
	install -m 644 ipcalc.1 $(ROOT)$(mandir)/man1
	install -m 644 consoletype.1 $(ROOT)$(mandir)/man1
	install -m 644 rename_device.1 $(ROOT)$(mandir)/man1
	install -m 644 initlog.conf $(ROOT)/etc

# this daemon and initscript are useful for testing the up/down/status stuff
# not installed by default, only comes from sources.
install-test:
	install -m 755 testd $(ROOT)/usr/sbin/testd
	install -m 755 testdinit $(ROOT)/etc/init.d/testd

getkey: getkey.o
	$(CC) $(LDFLAGS) -o $@ $< -Wl,-Bstatic -lpopt -Wl,-Bdynamic

ipcalc: ipcalc.o
	$(CC) $(LDFLAGS) -o $@ $< -Wl,-Bstatic -lpopt -Wl,-Bdynamic

initlog: $(INITLOG_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(INITLOG_OBJS) -Wl,-Bstatic -lpopt -Wl,-Bdynamic

usleep: $(USLEEP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(USLEEP_OBJS) -Wl,-Bstatic -lpopt -Wl,-Bdynamic

ppp-watch: $(PPPWATCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(PPPWATCH_OBJS) -Wl,-Bstatic `pkg-config glib-2.0 --libs` -Wl,-Bdynamic

usernetctl.o: usernetctl.c
	$(CC) $(CFLAGS) -fPIE -c usernetctl.c -o usernetctl.o

usernetctl: usernetctl.c usernetctl.o
	$(CC) $(LDFLAGS) -pie -o $@ $@.o

shvar.o: shvar.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c shvar.c -o shvar.o

ppp-watch.o: ppp-watch.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -c ppp-watch.c -o ppp-watch.o

rename_device: rename_device.c
	$(CC) $(CFLAGS) `pkg-config glib-2.0 --cflags` -o $@ $< -Wl,-Bstatic `pkg-config glib-2.0 --libs` -Wl,-Bdynamic

kmodule: kmodule.o
	$(CC) $(LDFLAGS) -o $@ $< -Wl,-Bstatic -lpopt -Wl,-Bdynamic -lkudzu -lpci
