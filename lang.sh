# /etc/profile.d/lang.sh - set i18n stuff

sourced=0
for langfile in /etc/sysconfig/i18n $HOME/.i18n ; do
    [ -f $langfile ] && . $langfile && sourced=1
done    

if [ -n "$GDM_LANG" ]; then
    sourced=1
    LANG="$GDM_LANG"
fi

if [ "$sourced" = 1 ]; then
    [ -n "$LANG" ] && export LANG || unset LANG
    [ -n "$LC_CTYPE" ] && export LC_CTYPE || unset LC_CTYPE
    [ -n "$LC_COLLATE" ] && export LC_COLLATE || unset LC_COLLATE
    [ -n "$LC_MESSAGES" ] && export LC_MESSAGES || unset LC_MESSAGES
    [ -n "$LC_NUMERIC" ] && export LC_NUMERIC || unset LC_NUMERIC
    [ -n "$LC_MONETARY" ] && export LC_MONETARY || unset LC_MONETARY
    [ -n "$LC_TIME" ] && export LC_TIME || unset LC_TIME
    if [ -n "$LC_ALL" ]; then
       if [ "$LC_ALL" != "$LANG" ]; then
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
          export LINGUAS
       else
          unset LINGUAS
       fi
    else 
       unset LINGUAS
    fi
    [ -n "$_XKB_CHARSET" ] && export _XKB_CHARSET || unset _XKB_CHARSET

    if [ -n "$CHARSET" ]; then
	case $CHARSET in
	    8859-1|8859-2|8859-15|koi*)
                if [ "$TERM" = "linux" -a "`/sbin/consoletype`" = "vt" ]; then
                       echo -n -e '\033(K' 2>/dev/null > /proc/$$/fd/0
                fi
                ;;
        esac
    elif [ -n "$SYSFONTACM" ]; then
	case $SYSFONTACM in
	    iso01*|iso02*|iso15*|koi*|latin2-ucw*)
		if [ "$TERM" = "linux" -a "`/sbin/consoletype`" = "vt" ]; then
			echo -n -e '\033(K' 2>/dev/null > /proc/$$/fd/0
		fi
		;;
	esac
    fi

    unset SYSFONTACM SYSFONT
fi
