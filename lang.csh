# /etc/profile.d/lang.csh - set i18n stuff

set sourced=0
foreach file (/etc/sysconfig/i18n $HOME/.i18n)
	if ( -f $file ) then
	    eval `sed 's|\([^=]*\)=\([^=]*\)|setenv \1 \2|g' $file | sed 's|$|;|' `
	endif
	set sourced=1
end

if ($sourced == 1) then
    if ($?LC_ALL && $?LANG) then
        if ($LC_ALL == $LANG) then
            unsetenv LC_ALL
        endif
    endif
    if ($?LINGUAS && $?LANG) then
        if ($LINGUAS == $LANG) then
            unsetenv LINGUAS
        endif
    endif

    if ($?SYSFONTACM) then
        switch ($SYSFONTACM)
	    case iso01*:
	    case iso02*:
	    case iso15*:
	    case koi*:
	    case latin2-ucw*:
	        if ( $?TERM ) then
		    if ( "$TERM" == "linux" ) then
		        if ( `/sbin/consoletype` == "vt" ) then
			    /bin/echo -n -e '\033(K' > /proc/$$/fd/15
		        endif
		    endif
		endif
		breaksw
	endsw
    endif
    unsetenv SYSFONTACM
    unsetenv SYSFONT
endif
