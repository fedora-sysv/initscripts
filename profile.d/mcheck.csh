
if ( -f /etc/sysconfig/mcheck ) then
    setenv MALLOC_CHECK_ = `grep "^MALLOC_CHECK_=" /etc/sysconfig/mcheck | cut -d"=" -f2`
endif
