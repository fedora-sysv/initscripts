# Enable 256 color capabilities for appropriate terminals

# Uncomment this if you want remote xterms connecting
# to this system, to be sent 256 colors.  Note that
# local xterms, ssh'ing to localhost are considered
# remote in this context, but in that case, $TERM should
# have already been set to 256 color capable.
#   SEND_256_COLORS_TO_REMOTE=1

# Terminals with any of the following set, support 256 colors (and are local)
set local256="$?COLORTERM$?XTERM_VERSION$?ROXTERM_ID$?KONSOLE_DBUS_SESSION"

if ($?TERM && ($local256 || $?SEND_256_COLORS_TO_REMOTE)) then

  switch ($TERM)
    case 'xterm':
    case 'screen':
    case 'Eterm':
      setenv TERM "$TERM-256color"
  endsw

  if ($?TERMCAP && ($TERM == "screen-256color")) then
    setenv TERMCAP `echo "$TERMCAP" | sed -e 's/Co#8/Co#256/g'`
  endif
endif

unset local256
