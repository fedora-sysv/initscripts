
if [ -f /etc/sysconfig/mcheck ]; then
    . /etc/sysconfig/mcheck
    if [ -n "$MALLOC_CHECK_" ]; then
        export MALLOC_PERTURB_=$(($RANDOM % 255 + 1))
    fi
fi
