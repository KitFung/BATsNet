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
CLUSTER=${NAME_1}=http://${HOST_1}:2380
DATA_DIR=etcd-data

# For node 1
THIS_NAME=${NAME_1}
THIS_IP=${HOST_1}
docker run \
  --restart=always \
  -d \
  -p 2379:2379 \
  -p 2380:2380 \
  --volume=${DATA_DIR}:/etcd-data \
  --name etcd ${REGISTRY}:${ETCD_VERSION} \
  /usr/local/bin/etcd \
  --data-dir=/etcd-data --name ${THIS_NAME} \
  --initial-advertise-peer-urls http://${THIS_IP}:2380 --listen-peer-urls http://0.0.0.0:2380 \
  --advertise-client-urls http://${THIS_IP}:2379 --listen-client-urls http://0.0.0.0:2379 \
  --initial-cluster ${CLUSTER} \
  --initial-cluster-state ${CLUSTER_STATE} --initial-cluster-token ${TOKEN}
