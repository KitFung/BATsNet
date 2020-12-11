VERSION=0.0.1

go mod vendor
docker build -t acl_proxy -f Dockerfile ./
docker tag acl_proxy:latest acl_proxy:v${VERSION}
docker tag acl_proxy:v${VERSION} 137.189.97.26:5000/acl_proxy:v${VERSION}
docker push 137.189.97.26:5000/acl_proxy:v${VERSION}
