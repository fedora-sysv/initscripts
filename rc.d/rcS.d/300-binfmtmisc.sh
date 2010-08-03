#!/bin/sh
# Set up binfmt_misc
/bin/mount -t binfmt_misc none /proc/sys/fs/binfmt_misc > /dev/null 2>&1
:
