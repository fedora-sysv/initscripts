#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions

# Fix console loglevel
if [ -n "$LOGLEVEL" ]; then
	/bin/dmesg -n $LOGLEVEL
fi
:
