#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

if strstr "$cmdline" confirm ; then
	touch /var/run/confirm
fi
