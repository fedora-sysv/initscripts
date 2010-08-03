#!/bin/sh
HOSTNAME=$(/bin/hostname)

if [ -f /etc/sysconfig/network ]; then
    . /etc/sysconfig/network
fi
if [ -z "$HOSTNAME" -o "$HOSTNAME" = "(none)" ]; then
    HOSTNAME=localhost
fi
hostname ${HOSTNAME}
:
