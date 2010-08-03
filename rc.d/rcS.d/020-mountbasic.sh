#!/bin/sh
if [ ! -e /proc/mounts ]; then
	mount -n -t proc /proc /proc
	mount -n -t sysfs /sys /sys >/dev/null 2>&1
fi
if [ ! -d /proc/bus/usb ]; then
	modprobe usbcore >/dev/null 2>&1 && mount -n -t usbfs /proc/bus/usb /proc/bus/usb
else
	mount -n -t usbfs /proc/bus/usb /proc/bus/usb
fi
:
