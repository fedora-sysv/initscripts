#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)
if [ -x /sbin/lvm ]; then
	action $"Setting up Logical Volume Management:" /sbin/lvm vgchange -a y --sysinit
fi
:
