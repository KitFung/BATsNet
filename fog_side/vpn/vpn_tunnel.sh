#!/bin/bash

USER=''
PASSWORD=''

OPENCONNECT_PID=""
function checkOpenconnect(){
    ps -p "${OPENCONNECT_PID}"
}

function startOpenConnect(){
    # start here open connect with your params and grab its pid
    # In root mode
    echo $PASSWORD | openconnect -u $USER --protocol=gp --passwd-on-stdin sslvpn-ie.cuhk.edu.hk &
}

startOpenConnect

TIME_INTERVAL=30

while true
do
    # sleep a bit of time
    sleep $TIME_INTERVAL
    OPENCONNECT_STATUS=$(checkOpenconnect)
    [ $OPENCONNECT_STATUS -ne 0 ] && startOpenConnect
    python3 report_ip.py $TIME_INTERVAL
done

# https://stackoverflow.com/questions/27940254/openconnect-autoconnect-reconnect-script
