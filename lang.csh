# /etc/profile.d/lang.csh - set i18n stuff

test -f /etc/sysconfig/i18n
if ($status == 0) then
    eval `sed 's|=C$|=en_US|g' /etc/sysconfig/i18n | sed 's|\([^=]*\)=\([^=]*\)|setenv \1 \2|g' | sed 's|$|;|' `

    if ($?SYSFONTACM) then
        switch ($SYSFONTACM)
	    case iso01*|iso02*|iso15*|koi*|latin2-ucw*:
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
