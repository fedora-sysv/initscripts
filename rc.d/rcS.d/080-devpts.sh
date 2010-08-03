#!/bin/sh
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

mount -n /dev/pts >/dev/null 2>&1
[ -n "$SELINUX_STATE" ] && restorecon /dev/pts >/dev/null 2>&1
:
