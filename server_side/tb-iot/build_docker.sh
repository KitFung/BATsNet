VERSION=0.0.1

go mod vendor
docker build -t tbmaster -f Dockerfile ./
docker tag tbmaster:latest tbmaster:v${VERSION}
docker tag tbmaster:v${VERSION} 137.189.97.26:5000/tbmaster:v${VERSION}
docker push 137.189.97.26:5000/tbmaster:v${VERSION}
