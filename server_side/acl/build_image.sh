VERSION=0.0.1

go mod vendor
docker build -t acl -f Dockerfile ./
docker tag acl:latest acl:v${VERSION}
docker tag acl:v${VERSION} 137.189.97.26:5000/acl:v${VERSION}
docker push 137.189.97.26:5000/acl:v${VERSION}
