VERSION=0.0.1

go mod vender
docker build -t cuhk-sensor-plugin -f Dockerfile ./
docker tag cuhk-sensor-plugin:latest cuhk-sensor-plugin:v${VERSION}
docker tag cuhk-sensor-plugin:v${VERSION} 137.189.97.26:5000/cuhk-sensor-plugin:v${VERSION}
docker push 137.189.97.26:5000/cuhk-sensor-plugin:v${VERSION}
