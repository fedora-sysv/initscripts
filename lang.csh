# /etc/profile.d/lang.csh - set i18n stuff

set sourced=0
if ($?LANG) then
    set sourced=1
else
    foreach file (/etc/sysconfig/i18n $HOME/.i18n)
	if ( -f $file ) then
	    eval `grep -v '^[:blank:]*#' $file | sed 's|\([^=]*\)=\([^=]*\)|setenv \1 \2|g' | sed 's|$|;|'`
	endif
	set sourced=1
    end
endif

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
    
    set consoletype=`/sbin/consoletype`

    if ($?LANG) then
        switch ($LANG)
	    case *.utf8*:
	    case *.UTF-8*:
		if ( $?TERM ) then
		    if ( "$TERM" == "linux" ) then
			if ( "$consoletype" == "vt" ) then
			    switch ($LANG)
			    	case en_IN*:
			    		breaksw
			    	case ja*:
			    	case ko*:
			    	case si*:
			    	case zh*:
			    	case *_IN*:
			    		setenv LANG en_US.UTF-8
			    		breaksw
			    endsw
			endif
		    endif
		endif
		breaksw
	    case *:
		if ( $?TERM ) then
		    if ( "$TERM" == "linux" ) then
			if ( "$consoletype" == "vt" ) then
			    switch ($LANG)
			    	case en_IN*:
			    		breaksw
			    	case ja*:
			    	case ko*:
			    	case si*:
			    	case zh*:
			    	case *_IN*:
			    		setenv LANG en_US
			    		breaksw
			    endsw
			endif
		    endif
		endif
		breaksw
	endsw
    endif    
    unsetenv SYSFONTACM
    unsetenv SYSFONT
endif
