FROM kitfung/batsnet:v0.0.1

RUN apt update && apt install -y python3-opencv

WORKDIR /opt

COPY . /opt/image_detection

ENTRYPOINT [ "bash", "/opt/image_detection/run.sh" ]
