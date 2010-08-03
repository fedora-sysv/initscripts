#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

if [ -x /sbin/quotacheck ] && { strstr "$cmdline" forcequotacheck || [ -f /forcequotacheck ]; } ; then
	action $"Checking local filesystem quotas: " /sbin/quotacheck -anug
fi

if [ -x /sbin/quotaon ]; then
    action $"Enabling local filesystem quotas: " /sbin/quotaon -aug
fi
:
