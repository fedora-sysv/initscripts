# /etc/profile.d/lang.csh - set i18n stuff

set sourced=0
foreach file (/etc/sysconfig/i18n $HOME/.i18n)
	if ( -f $file ) then
	    eval `grep -v '^[:blank:]*#' $file | sed 's|\([^=]*\)=\([^=]*\)|setenv \1 \2|g' | sed 's|$|;|'`
	endif
	set sourced=1
end

if ($?GDM_LANG) then
    set sourced=1
    set LANG=$GDM_LANG
endif

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

    if ($?CHARSET) then
        switch ($CHARSET)
            case 8859-1:
            case 8859-2:
            case 8859-15:
            case koi*:
            case latin2*:
                if ( $?TERM ) then
                    if ( "$TERM" == "linux" ) then
                        if ( `/sbin/consoletype` == "vt" ) then
                            /bin/echo -n -e '\033(K' >/proc/$$/fd/15
                        endif
                    endif
                endif
                breaksw
	endsw
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

switch ($LANG)
	case ja*:
	case zh*:
	case ko*:
	     set dspmpbyte=euc
	     breaksw
endsw
