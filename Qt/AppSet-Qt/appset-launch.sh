#! /bin/sh

PROGRAM_TO_LAUNCH=$1

: ${PROGRAM_TO_LAUNCH:="appset-qt"}

if [ -e /usr/bin/kdesu ]; then
    kdesu -d --noignorebutton -i "/usr/share/icons/appset/appset.png" -c $PROGRAM_TO_LAUNCH &
elif [ -e /usr/bin/gksu ]; then
    gksu -D "/usr/share/applications/appset-qt.desktop" $PROGRAM_TO_LAUNCH &
else
    xterm -e "sudo $PROGRAM_TO_LAUNCH"
fi
