#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions

if [ -f /etc/crypttab ]; then
    init_crypto 1
fi
:
