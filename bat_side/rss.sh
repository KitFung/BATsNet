docker rm -f rss

docker run -d \
    --restart always \
    --network=host \
    --name=rss aler9/rtsp-simple-server
