# /etc/profile.d/lang.csh - set i18n stuff

set sourced=0
foreach file (/etc/sysconfig/i18n $HOME/.i18n)
	if ( -f $file ) then
	    eval `sed 's|\([^=]*\)=\([^=]*\)|setenv \1 \2|g' $file | sed 's|$|;|' `
	endif
	set sourced=1
end

if ($sourced == 1) then
    if ($?LC_ALL && $?LANG) then
        if ($LC_ALL == $LANG) then
            unsetenv LC_ALL
        endif
    endif
    if ($?LINGUAS && $?LANG) then
        if ($LINGUAS == $LANG) then
            unsetenv LINGUAS
        endif
    endif

endif
