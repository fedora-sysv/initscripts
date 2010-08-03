#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

READONLY=no
if [ -f /etc/sysconfig/readonly-root ]; then
	. /etc/sysconfig/readonly-root
fi
if strstr "$cmdline" readonlyroot ; then
	READONLY=yes
	[ -z "$RW_MOUNT" ] && RW_MOUNT=/var/lib/stateless/writable
	[ -z "$STATE_MOUNT" ] && STATE_MOUNT=/var/lib/stateless/state
fi
if strstr "$cmdline" noreadonlyroot ; then
	READONLY=no
fi

# Check SELinux status
SELINUX_STATE=
if [ -e "/selinux/enforce" ] && [ "$(cat /proc/self/attr/current)" != "kernel" ]; then
	if [ -r "/selinux/enforce" ] ; then
		SELINUX_STATE=$(cat "/selinux/enforce")
	else
		# assume enforcing if you can't read it
		SELINUX_STATE=1
	fi
fi

[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

relabel_selinux() {
    # if /sbin/init is not labeled correctly this process is running in the
    # wrong context, so a reboot will be required after relabel
    AUTORELABEL=
    . /etc/selinux/config
    echo "0" > /selinux/enforce
    [ type -p plymouth &>/dev/null ] && plymouth --hide-splash

    if [ "$AUTORELABEL" = "0" ]; then
	echo
	echo $"*** Warning -- SELinux ${SELINUXTYPE} policy relabel is required. "
	echo $"*** /etc/selinux/config indicates you want to manually fix labeling"
	echo $"*** problems. Dropping you to a shell; the system will reboot"
	echo $"*** when you leave the shell."
	sulogin

    else
	echo
	echo $"*** Warning -- SELinux ${SELINUXTYPE} policy relabel is required."
	echo $"*** Relabeling could take a very long time, depending on file"
	echo $"*** system size and speed of hard drives."

	/sbin/fixfiles -F restore > /dev/null 2>&1
    fi
    rm -f  /.autorelabel
    echo $"Unmounting file systems"
    umount -a
    mount -n -o remount,ro /
    echo $"Automatic reboot in progress."
    reboot -f
}

# Check to see if a full relabel is needed
if [ -n "$SELINUX_STATE" -a "$READONLY" != "yes" ]; then
    if strstr "$cmdline" autorelabel || [ -f /.autorelabel ] ; then
	relabel_selinux
    fi
else
    if [ "$READONLY" != "yes" ] && [ -d /etc/selinux ]; then
        [ -f /.autorelabel ] || touch /.autorelabel
    fi
fi
:
