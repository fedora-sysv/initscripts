if [ -f /etc/sysconfig/i18n ]; then
        . /etc/sysconfig/i18n

  if [ -n "$LANG" ]; then
          export LANG
  fi
  
  if [ -n "$LINGUAS" ]; then
          export LINGUAS
  fi
  
  if [ -n "$SYSTERM" ]; then
          export TERM=$SYSTERM
  fi
fi
