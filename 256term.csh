# Enable 256 color capabilities for appropriate terminals

# Set this variable in your local shell config if you want remote
# xterms connecting to this system, to be sent 256 colors.
# This can be done in /etc/csh.cshrc, or in an earlier profile.d script.
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
    setenv TERMCAP `echo $TERMCAP | sed -e 's/Co#8/Co#256/g'`
  endif
endif

unset local256
