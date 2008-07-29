if ( -f /etc/sysconfig/debug ) then
	eval `egrep -v '(^[:blank:]*#|\$RANDOM)' /etc/sysconfig/debug | sed 's|^export ||g' | sed 's|\([^=]*\)=\([^=]*\)|setenv \1 \2|g' | sed 's|$|;|'`
endif
