#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

# Check SELinux status
SELINUX_STATE=
if [ -e "/selinux/enforce" ] && [ "$(cat /proc/self/attr/current)" != "kernel" ]; then
	if [ -r "/selinux/enforce" ] ; then
		SELINUX_STATE=$(cat "/selinux/enforce")
	else
		# assume enforcing if you can't read it
		SELINUX_STATE=1
	fi
fi

# Clean out /.
rm -f /fastboot /fsckoptions /forcefsck /.autofsck /forcequotacheck /halt \
	/poweroff /.suspended &> /dev/null

# Do we need (w|u)tmpx files? We don't set them up, but the sysadmin might...
_NEED_XFILES=
[ -f /var/run/utmpx ] || [ -f /var/log/wtmpx ] && _NEED_XFILES=1

# Clean up /var.
rm -rf /var/lock/cvs/* /var/run/screen/*
find /var/lock /var/run ! -type d -exec rm -f {} \;
rm -f /var/lib/rpm/__db* &> /dev/null
rm -f /var/gdm/.gdmfifo &> /dev/null


# Clean up utmp/wtmp
> /var/run/utmp
touch /var/log/wtmp
chgrp utmp /var/run/utmp /var/log/wtmp
chmod 0664 /var/run/utmp /var/log/wtmp
if [ -n "$_NEED_XFILES" ]; then
  > /var/run/utmpx
  touch /var/log/wtmpx
  chgrp utmp /var/run/utmpx /var/log/wtmpx
  chmod 0664 /var/run/utmpx /var/log/wtmpx
fi
[ -n "$SELINUX_STATE" ] && restorecon /var/run/utmp* /var/log/wtmp* >/dev/null 2>&1

# Clean up various /tmp bits
[ -n "$SELINUX_STATE" ] && restorecon /tmp
rm -f /tmp/.X*-lock /tmp/.lock.* /tmp/.gdm_socket /tmp/.s.PGSQL.*
rm -rf /tmp/.X*-unix /tmp/.ICE-unix /tmp/.font-unix /tmp/hsperfdata_* \
       /tmp/kde-* /tmp/ksocket-* /tmp/mc-* /tmp/mcop-* /tmp/orbit-*  \
       /tmp/scrollkeeper-*  /tmp/ssh-* \
       /dev/.in_sysinit

# Make ICE directory
mkdir -m 1777 -p /tmp/.ICE-unix >/dev/null 2>&1
chown root:root /tmp/.ICE-unix
[ -n "$SELINUX_STATE" ] && restorecon /tmp/.ICE-unix >/dev/null 2>&1
:
