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

    if [ -n "$SYSTERM" ] ; then
	case $SYSTERM in
	    linux-lat)
		LESSCHARSET=latin1
		INPUTRC=/etc/inputrc
		export LESSCHARSET INPUTRC
		;;
	esac
    fi
fi
