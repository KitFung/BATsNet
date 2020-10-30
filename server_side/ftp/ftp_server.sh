docker rm -f vsftpd

docker run -d -v /home/testbed/ftp/data:/home/vsftpd \
    -p 20:20 -p 21:21 -p 21100-21110:21100-21110 \
    -e FTP_USER=nvidia -e FTP_PASS=nvidia \
    -e PASV_ADDRESS=137.189.97.26 -e PASV_MIN_PORT=21100 \
    -e PASV_MAX_PORT=21110 \
    --name vsftpd --restart=always fauria/vsftpd
