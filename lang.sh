# /etc/profile.d/lang.sh - set i18n stuff

sourced=0
for langfile in /etc/sysconfig/i18n $HOME/.i18n ; do
    [ -f $langfile ] && . $langfile && sourced=1
done    

if [ -n "$GDM_LANG" ]; then
    sourced=1
    LANG="$GDM_LANG"
    unset LANGUAGE
    if [ "$GDM_LANG" = "zh_CN.GB18030" ]; then
      export LANGUAGE="zh_CN.GB18030:zh_CN.GB2312:zh_CN"
    fi
fi

if [ "$sourced" = 1 ]; then
    [ -n "$LANG" ] && export LANG || unset LANG
    [ -n "$LC_ADDRESS" ] && export LC_ADDRESS || unset LC_ADDRESS
    [ -n "$LC_CTYPE" ] && export LC_CTYPE || unset LC_CTYPE
    [ -n "$LC_COLLATE" ] && export LC_COLLATE || unset LC_COLLATE
    [ -n "$LC_IDENTIFICATION" ] && export LC_IDENTIFICATION || unset LC_IDENTIFICATION
    [ -n "$LC_MEASUREMENT" ] && export LC_MEASUREMENT || unset LC_MEASUREMENT
    [ -n "$LC_MESSAGES" ] && export LC_MESSAGES || unset LC_MESSAGES
    [ -n "$LC_MONETARY" ] && export LC_MONETARY || unset LC_MONETARY
    [ -n "$LC_NAME" ] && export LC_NAME || unset LC_NAME
    [ -n "$LC_NUMERIC" ] && export LC_NUMERIC || unset LC_NUMERIC
    [ -n "$LC_PAPER" ] && export LC_PAPER || unset LC_PAPER
    [ -n "$LC_TELEPHONE" ] && export LC_TELEPHONE || unset LC_TELEPHONE
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
    [ -n "$LINGUAS" ] && export LINGUAS || unset LINGUAS
    [ -n "$_XKB_CHARSET" ] && export _XKB_CHARSET || unset _XKB_CHARSET

    if [ -n "$CHARSET" ]; then
	case $CHARSET in
	    8859-1|8859-2|8859-5|8859-15|koi*)
                if [ "$TERM" = "linux" -a "`/sbin/consoletype`" = "vt" ]; then
                       echo -n -e '\033(K' 2>/dev/null > /proc/$$/fd/0
                fi
                ;;
        esac
    elif [ -n "$SYSFONTACM" ]; then
	case $SYSFONTACM in
	    iso01*|iso02*|iso05*|iso15*|koi*|latin2-ucw*)
		if [ "$TERM" = "linux" -a "`/sbin/consoletype`" = "vt" ]; then
			echo -n -e '\033(K' 2>/dev/null > /proc/$$/fd/0
		fi
		;;
	esac
    fi
    if [ -n "$LANG" ]; then
      case $LANG in
    	*.utf8*|*.UTF-8*)
    	if [ "$TERM" = "linux" -a "`/sbin/consoletype`" = "vt" ]; then
		[ -x /bin/unicode_start ] && /sbin/consoletype fg && unicode_start $SYSFONT $SYSFONTACM
        fi
	;;
      esac
    fi

    unset SYSFONTACM SYSFONT
fi
unset sourced
unset langfile
