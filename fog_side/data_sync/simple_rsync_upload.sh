#!/bin/bash
source /etc/profile.d/fog_env.sh

service vpn_tunnel restart
sleep 10

cd /opt/aiot/buf
TIMESTAMP=$(date '+%Y-%m-%d-%H-%M-%S')
TAR_FILE=${NODE_NAME}-${TIMESTAMP}.tar

tar -cvf $TAR_FILE --remove-files */*

rsync -avzh -e 'ssh -p 2202' \
    $TAR_FILE testbed@137.189.97.26:/home/testbed/lamppost_data_backup
rm -rf $TAR_FILE

# rsync \
#     --remove-source-files \
#     -avzh -e 'ssh -p 2202' \
#     ./*.tar testbed@137.189.97.26:/home/testbed/lamppost_data_backup

