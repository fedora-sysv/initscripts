#!/bin/sh
# Now that we have all of our basic modules loaded and the kernel going,
# let's dump the syslog ring somewhere so we can find it later
[ -f /var/log/dmesg ] && mv -f /var/log/dmesg /var/log/dmesg.old
dmesg -s 131072 > /var/log/dmesg
:
