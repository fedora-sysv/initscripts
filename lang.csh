#!/bin/csh

test -f /etc/sysconfig/i18n
if ($status == 0) then
    cat /etc/sysconfig/i18n | sed "s|=| |g" | sed "s|^\([^#]\)|setenv \0|g" > /tmp/csh.$$
    source /tmp/csh.$$
    rm -f /tmp/csh.$$

    if ($?SYSFONTACM) then
        switch ($SYSFONTACM)
	    case iso01*|iso02*|iso15*|koi*:
		if ( "$TERM" == "linux" ) then
		    if ( ls -l /proc/$$/fd/0 2>/dev/null | grep -- '-> /dev/tty[0-9]*$' >/dev/null 2>&1)  then
			echo -n -e '\033(K' > /proc/$$/fd/0
		    endif
		endif
		breaksw
	endsw
    endif
    unsetenv SYSFONTACM
endif
