# /etc/profile.d/lang.csh - set i18n stuff

set sourced=0

if ($?LANG) then
    set saved_lang=$LANG
    if ( -f "$HOME/.i18n" ) then
	eval `sed -ne 's|^[[:blank:]]*\([^#=]\{1,\}\)=\([^=]*\)|setenv \1 \2;|p' "$HOME/.i18n"`
	set sourced=1
    endif
    setenv LANG $saved_lang
    unset saved_lang
else
    foreach file (/etc/locale.conf "$HOME/.i18n")
        if ( -f $file ) then
	    eval `sed -ne 's|^[[:blank:]]*\([^#=]\{1,\}\)=\([^=]*\)|setenv \1 \2;|p' $file`
	    set sourced=1
        endif
    end
endif

if ($sourced == 1) then
    if ($?LC_ALL && $?LANG) then
        if ($LC_ALL == $LANG) then
            unsetenv LC_ALL
        endif
    endif
    
    set consoletype=`/sbin/consoletype stdout`

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
			    	case ar*:
			    	case fa*:
			    	case he*:
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
			    	case ar*:
			    	case fa*:
			    	case he*:
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
    unsetenv consoletype
endif
