CFLAGS+=$(RPM_OPT_FLAGS) -Wall -D_GNU_SOURCE

PROGS=usernetctl doexec netreport testd usleep ipcalc initlog minilogd \
	getkey ppp-watch consoletype
PPPWATCH_OBJS=ppp-watch.o shvar.o
INITLOG_OBJS=initlog.o process.o
USLEEP_OBJS=usleep.o

mandir=/usr/share/man

all:	$(PROGS)

clean:
	rm -f $(PROGS) *.o

install:
	mkdir -p $(ROOT)/bin $(ROOT)/usr/sbin $(ROOT)$(mandir)/man{1,8} $(ROOT)/etc
	install -s -m 755 doexec $(ROOT)/bin/doexec
	install -s -m 755 usleep $(ROOT)/bin/usleep
	install -s -m 4755 usernetctl $(ROOT)/usr/sbin/usernetctl
	install -s -m 2755 netreport $(ROOT)/sbin/netreport
	install -s -m 755 ipcalc $(ROOT)/bin/ipcalc
	install -s -m 755 initlog $(ROOT)/sbin/initlog
	install -s -m 755 minilogd $(ROOT)/sbin/minilogd
	install -s -m 755 getkey $(ROOT)/sbin/getkey
	install -s -m 755 ppp-watch $(ROOT)/sbin/ppp-watch
	install -s -m 755 consoletype $(ROOT)/sbin/consoletype
	install -m 644 initlog.1 $(ROOT)$(mandir)/man1
	install -m 644 doexec.1 $(ROOT)$(mandir)/man1
	install -m 644 netreport.1 $(ROOT)$(mandir)/man1
	install -m 644 usleep.1 $(ROOT)$(mandir)/man1
	install -m 644 usernetctl.8 $(ROOT)$(mandir)/man8
	install -m 644 ppp-watch.8 $(ROOT)$(mandir)/man8
	install -m 644 ipcalc.1 $(ROOT)$(mandir)/man1
	install -m 644 consoletype.1 $(ROOT)$(mandir)/man1
	install -m 644 initlog.conf $(ROOT)/etc

# this daemon and initscript are useful for testing the up/down/status stuff
# not installed by default, only comes from sources.
install-test:
	install -s -m 755 testd $(ROOT)/usr/sbin/testd
	install -m 755 testdinit $(ROOT)/etc/rc.d/init.d/testd

ipcalc: ipcalc.o
	$(CC) $(LDFLAGS) -o $@ $< /usr/lib/libpopt.a

initlog: $(INITLOG_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(INITLOG_OBJS) /usr/lib/libpopt.a

usleep: $(USLEEP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(USLEEP_OBJS) /usr/lib/libpopt.a

ppp-watch: $(PPPWATCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(PPPWATCH_OBJS) /usr/lib/libglib.a
shvar.o: shvar.c
	$(CC) $(CFLAGS) `glib-config --cflags` -c shvar.c -o shvar.o
ppp-watch.o: ppp-watch.c
	$(CC) $(CFLAGS) `glib-config --cflags` -c ppp-watch.c -o ppp-watch.o
