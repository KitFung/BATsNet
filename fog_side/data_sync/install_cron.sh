cp data_sync /etc/cron.d/data_sync
mkdir -p /opt/aiot/data_sync/
cp simple_rsync_upload.sh /opt/aiot/data_sync/
sudo crontab -u root /etc/cron.d/data_sync
