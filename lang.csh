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
    setenv LANG $GDM_LANG
    if ($?LANGUAGE) then
      unsetenv LANGUAGE
    endif
    if ("$GDM_LANG" == "zh_CN.GB18030") then
      setenv LANGUAGE "zh_CN.GB18030:zh_CN.GB2312:zh_CN"
    endif
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
            case 8859-5:
            case 8859-15:
            case koi*:
            case latin2*:
                if ( $?TERM ) then
                    if ( "$TERM" == "linux" ) then
                        if ( `/sbin/consoletype` == "vt" ) then
                            /bin/echo -n -e '\033(K' >/dev/tty
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
	    case iso05*:
	    case iso15*:
	    case koi*:
	    case latin2-ucw*:
	        if ( $?TERM ) then
		    if ( "$TERM" == "linux" ) then
		        if ( `/sbin/consoletype` == "vt" ) then
			    /bin/echo -n -e '\033(K' > /dev/tty
		        endif
		    endif
		endif
		breaksw
	endsw
    endif
    if ($?LANG) then
        switch ($LANG)
	    case *.utf8*:
	    case *.UTF-8*:
		if ( $?TERM ) then
		    if ( "$TERM" == "linux" ) then
			if ( `/sbin/consoletype` == "vt" ) then
			    if ( $?SYSFONTACM ) then
			        unicode_start $SYSFONT $SYSFONTACM
			    else
			        unicode_start $SYSFONT
			    endif
			endif
		    endif
		endif
		breaksw
	endsw
    endif    
    unsetenv SYSFONTACM
    unsetenv SYSFONT
endif

if ($?LANG) then
    switch ($LANG)
	case ja*UTF-8:
	case zh*UTF-8:
	case ko*UTF-8:
	     set dspmbyte=utf8
	     breaksw
	case zh_TW*:
	     set dspmbyte=big5
	     breaksw
	case ja*:
	case ko*:
	case zh*:
	     set dspmbyte=euc
	     breaksw
    endsw
endif
