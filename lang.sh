# /etc/profile.d/lang.sh - set i18n stuff

if [ -f /etc/sysconfig/i18n ]; then
    . /etc/sysconfig/i18n
    if [ -n "$LANG" ] ; then
       [ "$LANG" = "C" ] && LANG="en_US"
       export LANG
    else
      unset LANG
    fi
    [ -n "$LC_CTYPE" ] && export LC_CTYPE || unset LC_CTYPE
    [ -n "$LC_COLLATE" ] && export LC_COLLATE || unset LC_COLLATE
    [ -n "$LC_MESSAGES" ] && export LC_MESSAGES || unset LC_MESSAGES
    [ -n "$LC_NUMERIC" ] && export LC_NUMERIC || unset LC_NUMERIC
    [ -n "$LC_MONETARY" ] && export LC_MONETARY || unset LC_MONETARY
    [ -n "$LC_TIME" ] && export LC_TIME || unset LC_TIME
    if [ -n "$LC_ALL" ]; then
       if [ "$LC_ALL" != "$LANG" ]; then
         [ "$LC_ALL" = "C" ] && LC_ALL="en_US"
         export LC_ALL
       else
         unset LC_ALL
       fi
    else
       unset LC_ALL
    fi
    [ -n "$LANGUAGE" ] && export LANGUAGE || unset LANGUAGE
    if [ -n "$LINGUAS" ]; then
       if [ "$LINGUAS" != "$LANG" ]; then
          [ "$LINGUAS" = "C" ] && LINGUAS="en_US"
          export LINGUAS
       else
          unset LINGUAS
       fi
    else 
       unset LINGUAS
    fi
    [ -n "$_XKB_CHARSET" ] && export _XKB_CHARSET || unset _XKB_CHARSET

    if [ -n "$SYSFONTACM" ]; then
	case $SYSFONTACM in
	    iso01*|iso02*|iso15*|koi*|latin2-ucw*)
		if [ "$TERM" = "linux" ]; then
		    if ls -l /proc/$$/fd/0 2>/dev/null | grep -- '-> /dev/tty[0-9]*$' >/dev/null 2>&1; then
			echo -n -e '\033(K' > /proc/$$/fd/0
		    fi
		fi
		;;
	esac
    fi

    unset SYSFONTACM
fi
