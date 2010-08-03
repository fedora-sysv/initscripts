#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions

# Configure machine if necessary.
if [ -f /.unconfigured ]; then
    if [ -x /usr/bin/rhgb-client ] && /usr/bin/rhgb-client --ping ; then
	/usr/bin/rhgb-client --quit
    fi

    if [ -x /usr/bin/system-config-keyboard ]; then
	/usr/bin/system-config-keyboard
    fi
    if [ -x /usr/bin/passwd ]; then
        /usr/bin/passwd root
    fi
    if [ -x /usr/sbin/system-config-network-tui ]; then
	/usr/sbin/system-config-network-tui
    fi
    if [ -x /usr/sbin/timeconfig ]; then
	/usr/sbin/timeconfig
    fi
    if [ -x /usr/sbin/authconfig-tui ]; then
	/usr/sbin/authconfig-tui --nostart
    fi
    if [ -x /usr/sbin/ntsysv ]; then
	/usr/sbin/ntsysv --level 35
    fi

    # Reread in network configuration data.
    if [ -f /etc/sysconfig/network ]; then
	. /etc/sysconfig/network

	# Reset the hostname.
	action $"Resetting hostname ${HOSTNAME}: " hostname ${HOSTNAME}
    fi

    rm -f /.unconfigured
fi
:
