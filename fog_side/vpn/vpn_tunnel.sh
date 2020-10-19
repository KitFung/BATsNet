#!/bin/bash
source /etc/profile.d/fog_env.sh

OPENCONNECT_PID=""
function checkOpenconnect(){
    ps -p "${OPENCONNECT_PID}" | tail -n +2
}

function startOpenConnect(){
    # start here open connect with your params and grab its pid
    # In root mode
    echo $VPN_PASSWORD | openconnect -u $VPN_USER --protocol=gp --passwd-on-stdin sslvpn.ie.cuhk.edu.hk &
    OPENCONNECT_PID=$!
}

startOpenConnect

TIME_INTERVAL=30

while true
do
    # sleep a bit of time
    sleep $TIME_INTERVAL
    OPENCONNECT_STATUS=$(checkOpenconnect)
    if [ ! -n "$OPENCONNECT_STATUS" ]
    then
        pkill openconnect
        startOpenConnect
    fi
    python3 report_ip.py $TIME_INTERVAL
done

# https://stackoverflow.com/questions/27940254/openconnect-autoconnect-reconnect-script
