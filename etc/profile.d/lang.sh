# /etc/profile.d/lang.sh - exports environment variables, and provides fallback
#                          for CJK languages that can't be displayed in console.

if [ -n "${LANG}" ]; then
    LANG_backup="${LANG}"
fi

for config in /etc/locale.conf "${HOME}/.i18n"; do
    # NOTE: We are using eval & sed here to avoid invoking of any commands & functions from those files.
    if [ -f "${config}" ]; then
        eval $(sed -r -e 's/^[[:blank:]]*([[:upper:]_]+)=([[:print:][:digit:]\._-]+|"[[:print:][:digit:]\._-]+")/export \1=\2/;t;d' ${config})
    fi
done

if [ -n "${LANG_backup}" ]; then
    LANG="${LANG_backup}"
fi

unset LANG_backup config

# ----------------------------------------------

# The LC_ALL is not supposed to be set in /etc/locale.conf according to 'man 5 locale.conf'.
# If it is set, then we we expect it is user's explicit override (most likely from ~/.i18n file).
# See 'man 7 locale' for more info about LC_ALL.
if [ -n "${LC_ALL}" ]; then
    if [ "${LC_ALL}" != "${LANG}" ]; then
        export LC_ALL
    else
        unset LC_ALL
    fi
fi

# The ${LANG} manipulation is necessary only in virtual terminal (a.k.a. console - /dev/tty*):
if [ -n "${LANG}" ] && [ "${TERM}" = 'linux' ] && tty | grep --quiet -e '/dev/tty'; then
    if grep --quiet -E -i -e '^.+\.utf-?8$' <<< "${LANG}"; then
        case ${LANG} in
            ja*)    LANG=en_US.UTF-8 ;;
            ko*)    LANG=en_US.UTF-8 ;;
            si*)    LANG=en_US.UTF-8 ;;
            zh*)    LANG=en_US.UTF-8 ;;
            ar*)    LANG=en_US.UTF-8 ;;
            fa*)    LANG=en_US.UTF-8 ;;
            he*)    LANG=en_US.UTF-8 ;;
            en_IN*) true             ;;
            *_IN*)  LANG=en_US.UTF-8 ;;
        esac
    else
        case ${LANG} in
            ja*)    LANG=en_US ;;
            ko*)    LANG=en_US ;;
            si*)    LANG=en_US ;;
            zh*)    LANG=en_US ;;
            ar*)    LANG=en_US ;;
            fa*)    LANG=en_US ;;
            he*)    LANG=en_US ;;
            en_IN*) true       ;;
            *_IN*)  LANG=en_US ;;
        esac
    fi

    # NOTE: We are not exporting the ${LANG} here again on purpose.
    #       If user starts GUI session from console manually, then
    #       the previously set LANG should be okay to use.
fi
