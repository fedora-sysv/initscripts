Summary: inittab and /etc/rc.d scripts
Name: initscripts
%define version 3.53
Version: %{version}
Copyright: GPL
Group: Base
Release: 1
Source: initscripts-%{version}.tar.gz
BuildRoot: /var/tmp/initbld
Requires: mingetty bash mktemp
Prereq: /sbin/chkconfig

%description
This package contains the scripts use to boot a system, change run
levels, and shut the system down cleanly. It also contains the scripts
that activate and deactivate most network interfaces.

%changelog

* Sun Apr 05 1998 Erik Troan <ewt@redhat.com>

- updated rc.sysinit to deal with kernel versions with release numbers

* Sun Mar 22 1998 Erik Troan <ewt@redhat.com>

- use ipcalc to calculate the netmask if one isn't specified

* Tue Mar 10 1998 Erik Troan <ewt@redhat.com>

- added and made use of ipcalc

* Tue Mar 10 1998 Erik Troan <ewt@redhat.com>

- removed unnecessary dhcp log from /tmp

* Mon Mar 09 1998 Erik Troan <ewt@redhat.com>

- if bootpc fails, take down the device

* Mon Mar 09 1998 Erik Troan <ewt@redhat.com>

- added check for mktemp failure

* Thu Feb 05 1998 Erik Troan <ewt@redhat.com>

- fixed support for user manageable cloned devices

* Mon Jan 12 1998 Michael K. Johnson <johnsonm@redhat.com>

- /sbin/ isn't always in $PATH, so call /sbin/route in ifup-routes

* Wed Dec 31 1997 Erik Troan <ewt@redhat.com>

- touch /var/lock/subsys/kerneld after cleaning out /var/lock/subsys
- the logic for when  /var/lock/subsys/kerneld is touched was backwards

* Tue Dec 30 1997 Erik Troan <ewt@redhat.com>

- tried to get /proc stuff right one more time (uses -t nonfs,proc now)
- added support for /fsckoptions
- changed 'yse' to 'yes' in KERNELD= line

* Tue Dec 09 1997 Erik Troan <ewt@redhat.com>

- set domainname to "" if none is specified in /etc/sysconfig/network
- fix /proc mounting to get it in /etc/mtab

* Mon Dec 08 1997 Michael K. Johnson <johnsonm@redhat.com>

- fixed inheritance for clone devices

* Fri Nov 07 1997 Erik Troan <ewt@redhat.com>

- added sound support to rc.sysinit

* Fri Nov 07 1997 Michael K. Johnson <johnsonm@redhat.com>

- Added missing "then" clause

* Thu Nov 06 1997 Michael K. Johnson <johnsonm@redhat.com>

- Fixed DEBUG option in ifup-ppp
- Fixed PPP persistance
- Only change IP forwarding if necessary

* Tue Oct 28 1997 Donnie Barnes <djb@redhat.com>

- removed the skeleton init script
- added the ability to 'nice' daemons

* Tue Oct 28 1997 Erik Troan <ewt@redhat.com>

- touch /var/lock/subsys/kerneld if it's running, and after mounting /var
- applied dhcp fix

* Thu Oct 23 1997 Donnie Barnes <djb@redhat.com>

- added status|restart to init scripts

* Thu Oct 23 1997 Michael K. Johnson <johnsonm@redhat.com>

- touch random seed file before chmod'ing it.

* Wed Oct 15 1997 Erik Troan <ewt@redhat.com>

- run domainname if NISDOMAIN is set 

* Wed Oct 15 1997 Michael K. Johnson <johnsonm@redhat.com>

- Make the random seed file mode 600.

* Tue Oct 14 1997 Michael K. Johnson <johnsonm@redhat.com>

- bring down ppp devices if ifdown-ppp is called while ifup-ppp is sleeping.

* Mon Oct 13 1997 Erik Troan <ewt@redhat.com>

- moved to new chkconfig conventions

* Sat Oct 11 1997 Erik Troan <ewt@redhat.com>

- fixed rc.sysinit for hwclock compatibility

* Thu Oct 09 1997 Erik Troan <ewt@redhat.com>

- run 'ulimit -c 0' before running scripts in daemon function

* Wed Oct 08 1997 Donnie Barnes <djb@redhat.com>

- added chkconfig support
- made all rc*.d symlinks have missingok flag

* Mon Oct 06 1997 Erik Troan <ewt@redhat.com>

- fixed network-scripts to allow full pathnames as config files
- removed some old 3.0.3 pcmcia device handling

* Wed Oct 01 1997 Michael K. Johnson <johnsonm@redhat.com>

- /var/run/netreport needs to be group-writable now that /sbin/netreport
  is setguid instead of setuid.

* Tue Sep 30 1997 Michael K. Johnson <johnsonm@redhat.com>

- Added network-functions to spec file.
- Added report functionality to usernetctl.
- Fixed bugs I introduced into usernetctl while adding clone device support.
- Clean up entire RPM_BUILD_ROOT directory in %clean.

* Mon Sep 29 1997 Michael K. Johnson <johnsonm@redhat.com>

- Clone device support in network scripts, rc scripts, and usernetctl.
- Disassociate from controlling tty in PPP and SLIP startup scripts,
  since they act as daemons.
- Spec file now provides start/stop symlinks, since they don't fit in
  the CVS archive.

* Tue Sep 23 1997 Donnie Barnes <djb@redhat.com>

- added mktemp support to ifup

* Thu Sep 18 1997 Donnie Barnes <djb@redhat.com>

- fixed some init.d/functions bugs for stopping httpd

* Tue Sep 16 1997 Donnie Barnes <djb@redhat.com>

- reworked status() to adjust for processes that change their argv[0] in
  the process table.  The process must still have it's "name" in the argv[0]
  string (ala sendmail: blah blah).

* Mon Sep 15 1997 Erik Troan <ewt@redhat.com>

- fixed bug in FORWARD_IPV4 support

* Sun Sep 14 1997 Erik Troan <ewt@redhat.com>

- added support for FORWARD_IPV4 variable

* Thu Sep 11 1997 Donald Barnes <djb@redhat.com>

- added status function to functions along with better killproc 
  handling.
- added /sbin/usleep binary (written by me) and man page
- changed BuildRoot to /var/tmp instead of /tmp

* Tue Jun 10 1997 Michael K. Johnson <johnsonm@redhat.com>

- /sbin/netreport sgid rather than suid.
- /var/run/netreport writable by group root.

- /etc/ppp/ip-{up|down} no longer exec their local versions, so
  now ifup-post and ifdown-post will be called even if ip-up.local
  and ip-down.local exist.

* Tue Jun 03 1997 Michael K. Johnson <johnsonm@redhat.com>

- Added missing -f to [ invocation in ksyms check.

* Fri May 23 1997 Michael K. Johnson <johnsonm@redhat.com>

- Support for net event notification:
  Call /sbin/netreport to request that SIGIO be sent to you whenever
  a network interface changes status (won't work for brining up SLIP
  devices).
  Call /sbin/netreport -r to remove the notification request.
- Added ifdown-post, and made all the ifdown scrips call it, and
  added /etc/ppp/ip-down script that calls /etc/ppp/ip-down.local
  if it exists, then calls ifdown-post.
- Moved ifup and ifdown to /sbin

* Tue Apr 15 1997 Michael K. Johnson <johnsonm@redhat.com>

- usernetctl put back in ifdown
- support for slaved interfaces

* Wed Apr 02 1997 Erik Troan <ewt@redhat.com>

- Created ifup-post from old ifup
- PPP, PLIP, and generic ifup use ifup-post

* Fri Mar 28 1997 Erik Troan <ewt@redhat.com>

- Added DHCP support
- Set hostname via reverse name lookup after configuring a networking
  device if the current hostname is (none) or localhost

* Tue Mar 18 1997 Erik Troan <ewt@redhat.com>

- Got rid of xargs dependency in halt script
- Don't mount /proc twice (unmount it in between)
- sulogin and filesystem unmounting only happened for a corrupt root 
  filesystem -- it now happens when other filesystems are corrupt as well

* Tue Mar 04 1997 Michael K. Johnson <johnsonm@redhat.com>

PPP fixes and additions

* Mon Mar 03 1997 Erik Troan <ewt@redhat.com>

Mount proc before trying to start kerneld so we can test for /proc/ksyms
properly.

* Wed Feb 26 1997 Michael K. Johnson <johnsonm@redhat.com>

Added MTU for PPP.

Put PPPOPTIONS at the end of the options string instead of at the
beginning so that they override other options.  Gives users more rope...

Don't do module-based stuff on non-module systems.  Ignore errors if
st module isn't there and we try to load it.

* Tue Feb 25 1997 Michael K. Johnson <johnsonm@redhat.com>

Changed ifup-ppp and ifdown-ppp not to use doexec, because the argv[0]
provided by doexec goes away when pppd gets swapped out.

ifup-ppp now sets remotename to the logical name of the device.
This will BREAK current PAP setups on netcfg-managed interfaces,
but we needed to do this to add a reasonable interface-specific
PAP editor to netcfg.

* Fri Feb 07 1997 Erik Troan <ewt@redhat.com>

1) Added usernetctl wrapper for user mode ifup and ifdown's and man page
2) Rewrote ppp and slip kill and retry code 
3) Added doexec and man page

%prep
%setup

%build
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc
make ROOT=$RPM_BUILD_ROOT install 
mkdir -p $RPM_BUILD_ROOT/var/run/netreport
chown root.root $RPM_BUILD_ROOT/var/run/netreport
chmod u=rwx,g=rwx,o=rx $RPM_BUILD_ROOT/var/run/netreport

for i in 0 1 2 3 4 5 6 ; do
  file=$RPM_BUILD_ROOT/etc/rc.d/rc$i.d
  mkdir $file
  chown root.root $file
  chmod u=rwx,g=rx,o=rx $file
done

# Can't store symlinks in a CVS archive
ln -s ../init.d/random $RPM_BUILD_ROOT/etc/rc.d/rc0.d/K80random
ln -s ../init.d/random $RPM_BUILD_ROOT/etc/rc.d/rc1.d/S20random
ln -s ../init.d/random $RPM_BUILD_ROOT/etc/rc.d/rc2.d/S20random
ln -s ../init.d/random $RPM_BUILD_ROOT/etc/rc.d/rc3.d/S20random
ln -s ../init.d/random $RPM_BUILD_ROOT/etc/rc.d/rc4.d/S20random
ln -s ../init.d/random $RPM_BUILD_ROOT/etc/rc.d/rc5.d/S20random
ln -s ../init.d/random $RPM_BUILD_ROOT/etc/rc.d/rc6.d/K80random

ln -s ../init.d/nfsfs $RPM_BUILD_ROOT/etc/rc.d/rc0.d/K95nfsfs
ln -s ../init.d/nfsfs $RPM_BUILD_ROOT/etc/rc.d/rc1.d/K95nfsfs
ln -s ../init.d/nfsfs $RPM_BUILD_ROOT/etc/rc.d/rc2.d/K95nfsfs
ln -s ../init.d/nfsfs $RPM_BUILD_ROOT/etc/rc.d/rc3.d/S15nfsfs
ln -s ../init.d/nfsfs $RPM_BUILD_ROOT/etc/rc.d/rc4.d/S15nfsfs
ln -s ../init.d/nfsfs $RPM_BUILD_ROOT/etc/rc.d/rc5.d/S15nfsfs
ln -s ../init.d/nfsfs $RPM_BUILD_ROOT/etc/rc.d/rc6.d/K95nfsfs

ln -s ../init.d/network $RPM_BUILD_ROOT/etc/rc.d/rc0.d/K97network
ln -s ../init.d/network $RPM_BUILD_ROOT/etc/rc.d/rc1.d/K97network
ln -s ../init.d/network $RPM_BUILD_ROOT/etc/rc.d/rc2.d/S10network
ln -s ../init.d/network $RPM_BUILD_ROOT/etc/rc.d/rc3.d/S10network
ln -s ../init.d/network $RPM_BUILD_ROOT/etc/rc.d/rc4.d/S10network
ln -s ../init.d/network $RPM_BUILD_ROOT/etc/rc.d/rc5.d/S10network
ln -s ../init.d/network $RPM_BUILD_ROOT/etc/rc.d/rc6.d/K97network

ln -s ../init.d/killall $RPM_BUILD_ROOT/etc/rc.d/rc0.d/K90killall
ln -s ../init.d/killall $RPM_BUILD_ROOT/etc/rc.d/rc6.d/K90killall

ln -s ../init.d/halt $RPM_BUILD_ROOT/etc/rc.d/rc0.d/S00halt
ln -s ../init.d/halt $RPM_BUILD_ROOT/etc/rc.d/rc6.d/S00reboot

ln -s ../init.d/single $RPM_BUILD_ROOT/etc/rc.d/rc1.d/S00single

ln -s ../rc.local $RPM_BUILD_ROOT/etc/rc.d/rc2.d/S99local
ln -s ../rc.local $RPM_BUILD_ROOT/etc/rc.d/rc3.d/S99local
ln -s ../rc.local $RPM_BUILD_ROOT/etc/rc.d/rc5.d/S99local


%post
if [ ! -f /var/log/wtmp ]; then
  touch /var/log/wtmp
fi

chkconfig --add random 
chkconfig --add nfsfs 
chkconfig --add network 

%postun
if [ $1 = 0 ]; then
  chkconfig --del random
  chkconfig --del nfsfs
  chkconfig --del network
fi

%clean
rm -rf $RPM_BUILD_ROOT

%files
%dir /etc/sysconfig/network-scripts
%config %verify(not md5 mtime size) /etc/adjtime
/etc/sysconfig/network-scripts/ifdown
%config /sbin/ifdown
%config /etc/sysconfig/network-scripts/ifdown-post
/etc/sysconfig/network-scripts/ifup
%config /sbin/ifup
%config /etc/sysconfig/network-scripts/network-functions
%config /etc/sysconfig/network-scripts/ifup-post
%config /etc/sysconfig/network-scripts/ifdhcpc-done
%config /etc/sysconfig/network-scripts/ifcfg-lo
%config /etc/sysconfig/network-scripts/ifdown-ppp
%config /etc/sysconfig/network-scripts/ifdown-sl
%config /etc/sysconfig/network-scripts/ifup-ppp
%config /etc/sysconfig/network-scripts/ifup-sl
%config /etc/sysconfig/network-scripts/ifup-routes
%config /etc/sysconfig/network-scripts/ifup-plip
%config /etc/inittab
%dir    /etc/rc.d
%config /etc/rc.d/rc.sysinit
%dir    /etc/rc.d/rc0.d
%config(missingok) /etc/rc.d/rc0.d/*
%dir    /etc/rc.d/rc1.d
%config(missingok) /etc/rc.d/rc1.d/*
%dir    /etc/rc.d/rc2.d
%config(missingok) /etc/rc.d/rc2.d/*
%dir    /etc/rc.d/rc3.d
%config(missingok) /etc/rc.d/rc3.d/*
%dir    /etc/rc.d/rc4.d
%config(missingok) /etc/rc.d/rc4.d/*
%dir    /etc/rc.d/rc5.d
%config(missingok) /etc/rc.d/rc5.d/*
%dir    /etc/rc.d/rc6.d
%config(missingok) /etc/rc.d/rc6.d/*
%dir    /etc/rc.d/init.d
%config(missingok) /etc/rc.d/init.d/*
%config /etc/rc.d/rc
%config /etc/rc.d/rc.local
/bin/doexec
/bin/ipcalc
/bin/usleep
/usr/sbin/usernetctl
/sbin/netreport
/usr/man/man1/doexec.1
/usr/man/man1/ipcalc.1
/usr/man/man1/usleep.1
/usr/man/man1/usernetctl.1
/usr/man/man1/netreport.1
%dir /var/run/netreport
%config /etc/ppp/ip-up
%config /etc/ppp/ip-down
