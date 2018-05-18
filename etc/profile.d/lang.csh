# /etc/profile.d/lang.csh - exports environment variables, and provides fallback
#                           for CJK languages that can't be displayed in console.

if (${?LANG}) then
    set LANG_backup=${LANG}
endif

foreach config (/etc/locale.conf "${HOME}/.i18n")
    if (-f "${file}") then
        # NOTE: We are using eval & sed here to avoid invoking of any commands & functions from those files.
        eval `sed -r -e 's/^[[:blank:]]*([[:upper:]_]+)=([[:print:][:digit:]\._-]+|"[[:print:][:digit:]\._-]+")/setenv \1 \2;/;t;d' ${config}`
    endif
end

if (${?LANG_backup}) then
    set LANG="${LANG_backup}"
endif

unset LANG_backup config

# ----------------------------------------------

# The LC_ALL is not supposed to be set in /etc/locale.conf according to 'man 5 locale.conf'.
# If it is set, then we we expect it is user's explicit override (most likely from ~/.i18n file).
# See 'man 7 locale' for more info about LC_ALL.
if (${?LC_ALL}) then
    if (${LC_ALL} != ${LANG}) then
        setenv LC_ALL
    else
        unsetenv LC_ALL
    endif
endif

# The ${LANG} manipulation is necessary only in virtual terminal (a.k.a. console - /dev/tty*):
set in_console=`tty | grep --quiet -e '/dev/tty'; echo $?`

if (${?LANG} && ${TERM} == 'linux' && in_console == 0) then
    set utf8_used=`echo ${LANG} | grep --quiet -E -i -e '^.+\.utf-?8$'; echo $?`

    if (${utf8_used} == 0) then
        switch (${LANG})
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
    else
        switch (${LANG})
            case en_IN*:
                breaksw
            case ja*:
            case ko*:
            case si*:
            case zh*:
            m case ar*:
            case fa*:
            case he*:
            case *_IN*:
                setenv LANG en_US
                breaksw
        endsw
    endif

    # NOTE: We are not exporting the ${LANG} here again on purpose.
    #       If user starts GUI session from console manually, then
    #       the previously set LANG should be okay to use.
endif

unset in_console utf8_used
