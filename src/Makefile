CFLAGS+=-Wall -D_GNU_SOURCE

PROGS=usernetctl doexec netreport testd usleep ipcalc initlog minilogd

all:	$(PROGS)

clean:
	rm -f $(PROGS)

install:
	mkdir -p $(ROOT)/bin $(ROOT)/usr/sbin $(ROOT)/usr/man/man1
	install -s -m 755 doexec $(ROOT)/bin/doexec
	install -s -m 755 usleep $(ROOT)/bin/usleep
	install -s -m 4755 usernetctl $(ROOT)/usr/sbin/usernetctl
	install -s -m 2755 netreport $(ROOT)/sbin/netreport
	install -s -m 755 ipcalc $(ROOT)/bin/ipcalc
	install -s -m 755 initlog $(ROOT)/sbin/initlog
	install -s -m 755 minilogd $(ROOT)/sbin/minilogd
	install -m 644 initlog.1 $(ROOT)/usr/man/man1
	install -m 644 doexec.1 $(ROOT)/usr/man/man1
	install -m 644 netreport.1 $(ROOT)/usr/man/man1
	install -m 644 usleep.1 $(ROOT)/usr/man/man1
	install -m 644 usernetctl.1 $(ROOT)/usr/man/man1
	install -m 644 ipcalc.1 $(ROOT)/usr/man/man1

# this daemon and initscript are useful for testing the up/down/status stuff
# not installed by default, only comes from sources.
install-test:
	install -s -m 755 testd $(ROOT)/usr/sbin/testd
	install -m 755 testdinit $(ROOT)/etc/rc.d/init.d/testd

ipcalc: ipcalc.o
	$(CC) $(LDFLAGS) -o $@ $< -lpopt

minilogd: minilogd.o
        $(CC) $(LDFLAGS) -o $@ $< 

initlog: initlog.o process.o
	$(CC) $(LDFLAGS) -o $@ $< -lpopt

clean:
	rm ipcalc initlog minilogd *.o core *~

