#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions

# Device mapper & related initialization
if ! __fgrep "device-mapper" /proc/devices >/dev/null 2>&1 ; then
       modprobe dm-mod >/dev/null 2>&1
fi
:
