#!/bin/bash

if [ -f /etc/sysconfig/i18n ]; then
    . /etc/sysconfig/i18n

    if [ -n "$LANG" ]; then
	export LANG
    fi

    if [ -n "$LC_ALL" ]; then
	export LC_ALL
    fi
  
    if [ -n "$LINGUAS" ]; then
	export LINGUAS
    fi
  
    if [ -n "$SYSTERM" ]; then
	export TERM=$SYSTERM
    fi

    # Set console font map.
    if [ -n "$UNIMAP" ]; then
	loadunimap $UNIMAP
    fi
    
    if [ -n "$SYSFONTACM" ]; then
        case $SYSFONTACN in
	   iso01*|iso02*|iso15*)
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

    if [ -n "$SYSTERM" ] ; then
	case $SYSTERM in
	    linux-lat)
		LESSCHARSET=latin1
		INPUTRC=/etc/inputrc
		export LESSCHARSET INPUTRC
		;;
	esac
    fi
    unset SYSFONTACM
fi
