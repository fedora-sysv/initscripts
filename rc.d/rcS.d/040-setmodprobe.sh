#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

# Initialize hardware
if [ -f /proc/sys/kernel/modprobe ]; then
   if ! strstr "$cmdline" nomodules && [ -f /proc/modules ] ; then
       sysctl -w kernel.modprobe="/sbin/modprobe" >/dev/null 2>&1
   else
       # We used to set this to NULL, but that causes 'failed to exec' messages"
       sysctl -w kernel.modprobe="/bin/true" >/dev/null 2>&1
   fi
fi
:
