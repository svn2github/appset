#! /bin/sh

PROGRAM_TO_LAUNCH=$1

: ${PROGRAM_TO_LAUNCH:="appset-qt"}

#pgrep $PROGRAM_TO_LAUNCH > /dev/null

#if [ "$?" == "0" ]; then
#        appset-minimizer
#else
        if [ -e /usr/bin/kdesu ]; then
            kdesu -d --noignorebutton -i "/usr/share/icons/appset/appset.png" -c $PROGRAM_TO_LAUNCH $2 &
        elif [ -e /usr/bin/gksu ]; then
            gksu -D "/usr/share/applications/appset-qt.desktop" $PROGRAM_TO_LAUNCH $2 &
        elif [ -e /usr/bin/beesu ]; then
            beesu $PROGRAM_TO_LAUNCH $2 &
        else
            xterm -e "sudo $PROGRAM_TO_LAUNCH $2"
        fi
#fi


