#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

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

disable_selinux() {
	echo $"*** Warning -- SELinux is active"
	echo $"*** Disabling security enforcement for system recovery."
	echo $"*** Run 'setenforce 1' to reenable."
	echo "0" > "/selinux/enforce"
}

if [ -f /fsckoptions ]; then
	fsckoptions=$(cat /fsckoptions)
fi

if [ -f /forcefsck ] || strstr "$cmdline" forcefsck ; then
	fsckoptions="-f $fsckoptions"
elif [ -f /.autofsck ]; then
	[ -f /etc/sysconfig/autofsck ] && . /etc/sysconfig/autofsck
	if [ "$AUTOFSCK_DEF_CHECK" = "yes" ]; then
		AUTOFSCK_OPT="$AUTOFSCK_OPT -f"
	fi
	if [ -n "$AUTOFSCK_SINGLEUSER" ]; then
		[ type -p plymouth &>/dev/null ] && plymouth --hide-splash
		echo
		echo $"*** Warning -- the system did not shut down cleanly. "
		echo $"*** Dropping you to a shell; the system will continue"
		echo $"*** when you leave the shell."
		[ -n "$SELINUX_STATE" ] && echo "0" > /selinux/enforce
		sulogin
		[ -n "$SELINUX_STATE" ] && echo "1" > /selinux/enforce
		[ type -p plymouth &>/dev/null ] && plymouth --show-splash 
	fi
	fsckoptions="$AUTOFSCK_OPT $fsckoptions"
fi

if [ "$BOOTUP" = "color" ]; then
	fsckoptions="-C $fsckoptions"
else
	fsckoptions="-V $fsckoptions"
fi

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

if [ "$READONLY" = "yes" -o "$TEMPORARY_STATE" = "yes" ]; then

	mount_empty() {
		if [ -e "$1" ]; then
			echo "$1" | cpio -p -vd "$RW_MOUNT" &>/dev/null
			mount -n --bind "$RW_MOUNT$1" "$1"
		fi
	}

	mount_dirs() {
		if [ -e "$1" ]; then
			mkdir -p "$RW_MOUNT$1"
			find "$1" -type d -print0 | cpio -p -0vd "$RW_MOUNT" &>/dev/null
			mount -n --bind "$RW_MOUNT$1" "$1"
		fi
	}

	mount_files() {
		if [ -e "$1" ]; then
			cp -a --parents "$1" "$RW_MOUNT"
			mount -n --bind "$RW_MOUNT$1" "$1"
		fi
	}

	# Common mount options for scratch space regardless of
	# type of backing store
	mountopts=

	# Scan partitions for local scratch storage
	rw_mount_dev=$(blkid -t LABEL="$RW_LABEL" -l -o device)

	# First try to mount scratch storage from /etc/fstab, then any
	# partition with the proper label.  If either succeeds, be sure
	# to wipe the scratch storage clean.  If both fail, then mount
	# scratch storage via tmpfs.
	if mount $mountopts "$RW_MOUNT" > /dev/null 2>&1 ; then
		rm -rf "$RW_MOUNT" > /dev/null 2>&1
	elif [ x$rw_mount_dev != x ] && mount $rw_mount_dev $mountopts "$RW_MOUNT" > /dev/null 2>&1; then
		rm -rf "$RW_MOUNT"  > /dev/null 2>&1
	else
		mount -n -t tmpfs $RW_OPTIONS $mountopts none "$RW_MOUNT"
	fi

	for file in /etc/rwtab /etc/rwtab.d/* /dev/.initramfs/rwtab ; do
		is_ignored_file "$file" && continue
	[ -f $file ] && cat $file | while read type path ; do
			case "$type" in
				empty)
					mount_empty $path
					;;
				files)
					mount_files $path
					;;
				dirs)
					mount_dirs $path
					;;
				*)
					;;
			esac
			[ -n "$SELINUX_STATE" ] && [ -e "$path" ] && restorecon -R "$path"
		done
	done

	# Use any state passed by initramfs
	[ -d /dev/.initramfs/state ] && cp -a /dev/.initramfs/state/* $RW_MOUNT

	# In theory there should be no more than one network interface active
	# this early in the boot process -- the one we're booting from.
	# Use the network address to set the hostname of the client.  This
	# must be done even if we have local storage.
	ipaddr=
	if [ "$HOSTNAME" = "localhost" -o "$HOSTNAME" = "localhost.localdomain" ]; then
		ipaddr=$(ip addr show to 0.0.0.0/0 scope global | awk '/[[:space:]]inet / { print gensub("/.*","","g",$2) }')
		for ip in $ipaddr ; do
			HOSTNAME=
			eval $(ipcalc -h $ipaddr 2>/dev/null)
			[ -n "$HOSTNAME" ] && { hostname ${HOSTNAME} ; break; }
		done
	fi
	
	# Clients with read-only root filesystems may be provided with a
	# place where they can place minimal amounts of persistent
	# state.  SSH keys or puppet certificates for example.
	#
	# Ideally we'll use puppet to manage the state directory and to
	# create the bind mounts.  However, until that's all ready this
	# is sufficient to build a working system.

	# First try to mount persistent data from /etc/fstab, then any
	# partition with the proper label, then fallback to NFS
	state_mount_dev=$(blkid -t LABEL="$STATE_LABEL" -l -o device)
	if mount $mountopts $STATE_OPTIONS "$STATE_MOUNT" > /dev/null 2>&1 ; then
		/bin/true
	elif [ x$state_mount_dev != x ] && mount $state_mount_dev $mountopts "$STATE_MOUNT" > /dev/null 2>&1;  then
		/bin/true
	elif [ ! -z "$CLIENTSTATE" ]; then
		# No local storage was found.  Make a final attempt to find
		# state on an NFS server.

		mount -t nfs $CLIENTSTATE/$HOSTNAME $STATE_MOUNT -o rw,nolock
	fi

	if [ -w "$STATE_MOUNT" ]; then

		mount_state() {
			if [ -e "$1" ]; then
				[ ! -e "$STATE_MOUNT$1" ] && cp -a --parents "$1" "$STATE_MOUNT"
				mount -n --bind "$STATE_MOUNT$1" "$1"
			fi
		}

		for file in /etc/statetab /etc/statetab.d/* ; do
			is_ignored_file "$file" && continue
			[ ! -f "$file" ] && continue

			if [ -f "$STATE_MOUNT/$file" ] ; then
				mount -n --bind "$STATE_MOUNT/$file" "$file"
			fi

			for path in $(grep -v "^#" "$file" 2>/dev/null); do
				mount_state "$path"
				[ -n "$SELINUX_STATE" ] && [ -e "$path" ] && restorecon -R "$path"
			done
		done

		if [ -f "$STATE_MOUNT/files" ] ; then
			for path in $(grep -v "^#" "$STATE_MOUNT/files" 2>/dev/null); do
				mount_state "$path"
				[ -n "$SELINUX_STATE" ] && [ -e "$path" ] && restorecon -R "$path"
			done
		fi
	fi
fi

if [[ " $fsckoptions" != *" -y"* ]]; then
	fsckoptions="-a $fsckoptions"
fi

if [ -f /fastboot ] || strstr "$cmdline" fastboot ; then
	fastboot=yes
fi
if [ -z "$fastboot" -a "$READONLY" != "yes" ]; then

        STRING=$"Checking filesystems"
	echo $STRING
	fsck -T -t noopts=_netdev -A $fsckoptions
	rc=$?
	
	if [ "$rc" -eq "0" ]; then
		success "$STRING"
		echo
	elif [ "$rc" -eq "1" ]; then
	        passed "$STRING"
		echo
	elif [ "$rc" -eq "2" -o "$rc" -eq "3" ]; then
		echo $"Unmounting file systems"
		umount -a
		mount -n -o remount,ro /
		echo $"Automatic reboot in progress."
		reboot -f
        fi
	
        # A return of 4 or higher means there were serious problems.
	if [ $rc -gt 1 ]; then
	        [ type -p plymouth &>/dev/null ] && plymouth --hide-splash 

		failure "$STRING"
		echo
		echo
		echo $"*** An error occurred during the file system check."
		echo $"*** Dropping you to a shell; the system will reboot"
		echo $"*** when you leave the shell."

                str=$"(Repair filesystem)"
		PS1="$str \# # "; export PS1
		[ "$SELINUX_STATE" = "1" ] && disable_selinux
		sulogin

		echo $"Unmounting file systems"
		umount -a
		mount -n -o remount,ro /
		echo $"Automatic reboot in progress."
		reboot -f
	elif [ "$rc" -eq "1" ]; then
		_RUN_QUOTACHECK=1
	fi
fi

remount_needed() {
  local state oldifs
  [ "$READONLY" = "yes" ] && return 1
  state=$(LC_ALL=C awk '/ \/ / && ($3 !~ /rootfs/) { print $4 }' /proc/mounts)
  oldifs=$IFS
  IFS=","
  for opt in $state ; do
	if [ "$opt" = "rw" ]; then
		IFS=$oldifs
		return 1
	fi
  done
  IFS=$oldifs
  return 0
}

# Remount the root filesystem read-write.
update_boot_stage RCmountfs
if remount_needed ; then
  action $"Remounting root filesystem in read-write mode: " mount -n -o remount,rw /
fi

# Clean up SELinux labels
if [ -n "$SELINUX_STATE" ]; then
   restorecon /etc/mtab /etc/ld.so.cache /etc/blkid/blkid.tab /etc/resolv.conf >/dev/null 2>&1
fi

# If relabeling, relabel mount points.
if [ -n "$SELINUX_STATE" -a "$READONLY" != "yes" ]; then
    if strstr "$cmdline" autorelabel || [ -f /.autorelabel ] ; then
	restorecon $(awk '!/^#/ && $4 !~ /noauto/ && $2 ~ /^\// { print $2 }' /etc/fstab) >/dev/null 2>&1
    fi
fi

if [ "$READONLY" != "yes" ] ; then
	# Clear mtab
	(> /etc/mtab) &> /dev/null

	# Remove stale backups
	rm -f /etc/mtab~ /etc/mtab~~

	# Enter mounted filesystems into /etc/mtab
	mount -f /
	mount -f /proc >/dev/null 2>&1
	mount -f /sys >/dev/null 2>&1
	mount -f /dev/pts >/dev/null 2>&1
	mount -f /dev/shm >/dev/null 2>&1
	mount -f /proc/bus/usb >/dev/null 2>&1
fi

# Mount all other filesystems (except for NFS and /proc, which is already
# mounted). Contrary to standard usage,
# filesystems are NOT unmounted in single user mode.
if [ "$READONLY" != "yes" ] ; then
	action $"Mounting local filesystems: " mount -a -t nonfs,nfs4,smbfs,ncpfs,cifs,gfs,gfs2 -O no_netdev
else
	action $"Mounting local filesystems: " mount -a -n -t nonfs,nfs4,smbfs,ncpfs,cifs,gfs,gfs2 -O no_netdev
fi

[[ $_RUN_QUOTACHECK = 1 ]] && touch /forcequotacheck
:
