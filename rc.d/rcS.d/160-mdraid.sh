#!/bin/sh
# Start any MD RAID arrays that haven't been started yet
[ -r /proc/mdstat -a -r /dev/md/md-device-map ] && /sbin/mdadm -IRs
:
