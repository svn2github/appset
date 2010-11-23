#! /bin/sh

if [ -e /usr/bin/kdesu ]; then
    kdesu kdesu -d --noignorebutton -i "/usr/share/icons/appset/appset.png" -c $1 &
elif [ -e /usr/bin/gksu ]; then
    gksu -D "/usr/share/applications/appset-qt.desktop" $1 &
else
    xterm -e "sudo $1"
fi
