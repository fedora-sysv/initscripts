#!/bin/bash

if [ -f /etc/sysconfig/i18n ]; then
    . /etc/sysconfig/i18n
    [ -n "$LANG" ] && export LANG || unset LANG
    [ -n "$LC_CTYPE" ] && export LC_CTYPE || unset LC_CTYPE
    [ -n "$LC_COLLATE" ] && export LC_COLLATE || unset LC_COLLATE
    [ -n "$LC_MESSAGES" ] && export LC_MESSAGES || unset LC_MESSAGES
    [ -n "$LC_NUMERIC" ] && export LC_NUMERIC || unset LC_NUMERIC
    [ -n "$LC_MONETARY" ] && export LC_MONETARY || unset LC_MONETARY
    [ -n "$LC_TIME" ] && export LC_TIME || unset LC_TIME
    [ -n "$LC_ALL" ] && export LC_ALL || unset LC_ALL
    [ -n "$LANGUAGE" ] && export LANGUAGE || unset LANGUAGE
    [ -n "$LINGUAS" ] && export LINGUAS || unset LINGUAS

    # deprecated
    if [ -n "$SYSTERM" ]; then
	export TERM=$SYSTERM
    fi

    if [ -n "$SYSFONTACM" ]; then
        case $SYSFONTACM in
	   iso01*|iso02*|iso15*|koi*)
	        LESSCHARSET=latin1
		INPUTRC=/etc/inputrc
		export LESSCHARSET INPUTRC
		if [ "$TERM" = "linux" ]; then
		    if ls -l /proc/$$/fd/0 2>/dev/null | grep -- '-> /dev/tty[0-9]*$' >/dev/null 2>&1; then
			echo -n -e '\033(K' > /proc/$$/fd/0
		    fi
		fi
		;;
       esac
    fi
    
    if [ -n "$INPUTRC" ]; then
        export INPUTRC
    elif [ "$TERM" = "linux-lat" ]; then
        INPUTRC=/etc/inputrc
	export INPUTRC
    fi
    
    if [ -n "$LESSCHARSET" ]; then
        export LESSCHARSET
    elif [ "$TERM" = "linux-lat" ]; then
        LESSCHARSET=latin1
	export LESSCHARSET
    fi
    
    [ -n "$_XKB_CHARSET" ] && export _XKB_CHARSET || unset _XKB_CHARSET

    unset SYSFONTACM
fi
