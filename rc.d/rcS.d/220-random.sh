#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

READONLY=no
if [ -f /etc/sysconfig/readonly-root ]; then
	. /etc/sysconfig/readonly-root
fi
if strstr "$cmdline" readonlyroot ; then
	READONLY=yes
	[ -z "$RW_MOUNT" ] && RW_MOUNT=/var/lib/stateless/writable
	[ -z "$STATE_MOUNT" ] && STATE_MOUNT=/var/lib/stateless/state
fi
if strstr "$cmdline" noreadonlyroot ; then
	READONLY=no
fi

# Initialize pseudo-random number generator
if [ -f "/var/lib/random-seed" ]; then
	cat /var/lib/random-seed > /dev/urandom
else
	[ "$READONLY" != "yes" ] && touch /var/lib/random-seed
fi
if [ "$READONLY" != "yes" ]; then
	chmod 600 /var/lib/random-seed
	dd if=/dev/urandom of=/var/lib/random-seed count=1 bs=512 2>/dev/null
fi
:
