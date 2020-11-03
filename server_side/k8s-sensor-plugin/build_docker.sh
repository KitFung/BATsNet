# docker build -t k8s-sensor-plugin -f dev_Dockerfile ./
# docker run -it --rm -v $(pwd):/go/src/k8s-sensor-plugin k8s-sensor-plugin:latest bash


go mod vender
docker build -t cuhk-sensor-plugin -f Dockerfile ./
docker tag cuhk-sensor-plugin:latest cuhk-sensor-plugin:v0.0.1
docker tag cuhk-sensor-plugin:v0.0.1 137.189.97.26:5000/cuhk-sensor-plugin:v0.0.1
