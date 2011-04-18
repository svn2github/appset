#!/bin/bash

. /etc/rc.conf
. /etc/rc.d/functions

LOG="/var/log/appset-helper.log"

case "$1" in
        start)
                stat_busy "Starting appset-helper"

                nice -n 15 appset-helper &

                stat_done
        ;;

        stop)
                stat_busy "Stopping appset-helper"

                killall -e appset-helper > /dev/null 2> /dev/null
                rm /tmp/ashelper.tmp
                rm /tmp/ashelper.out

                stat_done

        ;;

        restart)
                $0 stop
                sleep 1
                $0 start
        ;;


        *)
        echo "Usage: $0 {start|stop|restart}"
esac
