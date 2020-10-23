docker rm -f etcd
docker volume rm -f etcd-data
docker volume create --name etcd-data
export DATA_DIR="etcd-data"

REGISTRY=gcr.io/etcd-development/etcd

# For each machine
ETCD_VERSION=latest
TOKEN=my-etcd-token
CLUSTER_STATE=new
NAME_1=etcd-server
HOST_1=137.189.97.26
CLUSTER=${NAME_1}=http://${HOST_1}:12380
DATA_DIR=etcd-data

# For node 1
THIS_NAME=${NAME_1}
THIS_IP=${HOST_1}
docker run \
  --restart=always \
  -d \
  -p 12379:12379 \
  -p 12380:12380 \
  --volume=${DATA_DIR}:/etcd-data \
  --name etcd ${REGISTRY}:${ETCD_VERSION} \
  /usr/local/bin/etcd \
  --data-dir=/etcd-data --name ${THIS_NAME} \
  --initial-advertise-peer-urls http://${THIS_IP}:12380 --listen-peer-urls http://0.0.0.0:12380 \
  --advertise-client-urls http://${THIS_IP}:12379 --listen-client-urls http://0.0.0.0:12379 \
  --initial-cluster ${CLUSTER} \
  --initial-cluster-state ${CLUSTER_STATE} --initial-cluster-token ${TOKEN}
