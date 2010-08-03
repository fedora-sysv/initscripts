#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

# Set default affinity
if [ -x /bin/taskset ]; then
   if strstr "$cmdline" default_affinity= ; then
     for arg in $cmdline ; do
         if [ "${arg##default_affinity=}" != "${arg}" ]; then
             /bin/taskset -p ${arg##default_affinity=} 1
         fi
     done
   fi
fi
:
