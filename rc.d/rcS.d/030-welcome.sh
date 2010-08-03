#!/bin/sh
[[ $(type -t strstr) = "function" ]] || . /etc/init.d/functions
[[ $cmdline ]] || cmdline=$(cat /proc/cmdline)

# Print a text banner.
echo -en $"\t\tWelcome to "
read -r system_release < /etc/system-release
if [[ "$system_release" == *"Red Hat"* ]]; then
 [ "$BOOTUP" = "color" ] && echo -en "\\033[0;31m"
 echo -en "Red Hat"
 [ "$BOOTUP" = "color" ] && echo -en "\\033[0;39m"
 PRODUCT=$(sed "s/Red Hat \(.*\) release.*/\1/" /etc/system-release)
 echo " $PRODUCT"
elif [[ "$system_release" == *Fedora* ]]; then
 [ "$BOOTUP" = "color" ] && echo -en "\\033[0;34m"
 echo -en "Fedora"
 [ "$BOOTUP" = "color" ] && echo -en "\\033[0;39m"
 PRODUCT=$(sed "s/Fedora \(.*\) \?release.*/\1/" /etc/system-release)
 echo " $PRODUCT"
else
 PRODUCT=$(sed "s/ release.*//g" /etc/system-release)
 echo "$PRODUCT"
fi
if [ "$PROMPT" != "no" ]; then
 echo -en $"\t\tPress 'I' to enter interactive startup."
 echo
fi
:
