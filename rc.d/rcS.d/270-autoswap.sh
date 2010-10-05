#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions

# Start up swapping.
if [ "$AUTOSWAP" = "yes" ]; then
	curswap=$(awk '/^\/dev/ { print $1 }' /proc/swaps | while read x; do get_numeric_dev dec $x ; echo -n " "; done)
	swappartitions=$(blkid -t TYPE=swap -o device)
	if [ x"$swappartitions" != x ]; then
		for partition in $swappartitions ; do
			[ ! -e $partition ] && continue
			majmin=$(get_numeric_dev dec $partition)
			echo $curswap | grep -qw "$majmin" || action $"Enabling local swap partitions: " swapon $partition
		done
	fi
fi
:
