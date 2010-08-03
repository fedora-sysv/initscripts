#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

if ! strstr "$cmdline" nodmraid && [ -x /sbin/dmraid ]; then
	modprobe dm-mirror >/dev/null 2>&1
	dmraidsets=$(LC_ALL=C /sbin/dmraid -s -c -i)
	if [ "$?" = "0" ]; then
		for dmname in $dmraidsets; do
			if [[ "$dmname" == isw_* ]] && \
			   ! strstr "$cmdline" noiswmd; then
				continue
			fi
			/sbin/dmraid -ay -i --rm_partitions -p "$dmname" >/dev/null 2>&1
			/sbin/kpartx -a -p p "/dev/mapper/$dmname"
		done
	fi
fi
:
