install ipv6 { . /etc/sysconfig/network ; [ -n "$NETWORKING_IPV6" -a "$NETWORKING_IPV6" != "yes"  ] || /sbin/modprobe --ignore-install ipv6 $CMDLINE_OPTS ; }
