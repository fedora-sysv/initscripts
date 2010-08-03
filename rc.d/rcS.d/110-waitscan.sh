#!/bin/sh
# Sync waiting for storage.
{ rmmod scsi_wait_scan ; modprobe scsi_wait_scan ; rmmod scsi_wait_scan ; } >/dev/null 2>&1
:
