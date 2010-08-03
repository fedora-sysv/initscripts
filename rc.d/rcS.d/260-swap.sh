#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions

# Start up swapping.
update_boot_stage RCswap
action $"Enabling /etc/fstab swaps: " swapon -a -e

:
