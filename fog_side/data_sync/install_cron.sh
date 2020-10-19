cp data_sync /etc/cron.d/data_sync
mkdir /opt/aiot/data_sync/
cp simple_rsync_upload.sh /opt/aiot/data_sync/
crontab -u /etc/cron.d/data_sync
