#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

if ! strstr "$cmdline" nompath && [ -f /etc/multipath.conf ] && \
		[ -x /sbin/multipath ]; then
	modprobe dm-multipath > /dev/null 2>&1
	/sbin/multipath -v 0
	if [ -x /sbin/kpartx ]; then
		/sbin/dmsetup ls --target multipath --exec "/sbin/kpartx -a -p p" >/dev/null
	fi
fi
:
