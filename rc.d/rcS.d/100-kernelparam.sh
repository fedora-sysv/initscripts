#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions

# Configure kernel parameters
update_boot_stage RCkernelparam
sysctl -e -p /etc/sysctl.conf >/dev/null 2>&1
:
