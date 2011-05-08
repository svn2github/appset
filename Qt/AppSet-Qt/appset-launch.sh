#! /bin/sh

if [ "$#" == "0" ]; then
    pgrep appset-qt > /dev/null
    if [ "$?" == "0" ]; then
        if [ -p /tmp/asmin ]; then
            echo h > /tmp/asmin
        fi
    else
        rm /tmp/asuser.tmp > /dev/null
        rm /tmp/asmin > /dev/null
        appset-qt &
    fi
elif [ "$1" == "--hidden" ]; then
    if [ "$?" -ne "0" ]; then
        appset-qt --hidden &
    fi
elif [ "$1" == "--show" ]; then
    pgrep appset-qt > /dev/null
    if [ "$?" == "0" ]; then
        if [ -p /tmp/asmin ]; then
            echo h > /tmp/asmin
        fi
    else
        rm /tmp/asuser.tmp > /dev/null
        appset-qt --show &
    fi
elif [ $# -ge 1 ]; then        
        if [ -e /usr/bin/kdesu ]; then
            kdesu -d --noignorebutton -i "/usr/share/icons/appset/appset.png" -c "appset-qt $1 $2 $3"
        elif [ -e /usr/bin/gksu ]; then
            gksu -D "/usr/share/applications/appset-qt.desktop" "appset-qt $1 $2 $3"
        elif [ -e /usr/bin/beesu ]; then
            beesu "appset-qt $1 $2 $3"
        elif [ -e /usr/bin/xdg-su ]; then
            xdg-su -c "appset-qt $1 $2 $3"
        else
            xterm -e "sudo appset-qt $1 $2 $3"
        fi
fi


