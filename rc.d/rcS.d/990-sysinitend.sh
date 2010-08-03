#!/bin/sh
# Let rhgb know that we're leaving rc.sysinit
if [ -x /usr/bin/rhgb-client ] && /usr/bin/rhgb-client --ping ; then
    /usr/bin/rhgb-client --sysinit
fi
:
