docker run --rm -d --net=host \
  --env ASSIGNED_PORT=9080 \
  --env FOG_ADDRESS=10.42.0.1 \
  --env FOG_PORT=50051 \
  --env ENVOY_ADDRESS=10.42.0.1:9080 \
  -v $(pwd):/grpc_proxy \
  --name test_proxy \
  kitfung/envoy_plus:v1.15-latest \
  /grpc_proxy/start.sh

  start.sh
