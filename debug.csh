if ( -f /etc/sysconfig/debug ) then
	eval `grep -Ev '(^[[:blank:]]*#|\$RANDOM)' /etc/sysconfig/debug | sed 's|^export ||g' | sed 's|\([^=]*\)=\([^=]*\)|setenv \1 \2|g' | sed 's|$|;|'`
endif
