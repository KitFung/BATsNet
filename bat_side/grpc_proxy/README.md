# The proxy service

Create a proxy for the device controller in the corresponding fog node.

It will also monitor the process health of the device controller by checking the BAT etcd database. The value of key: `$DEVICE_CONTROLLER_ERVICE_ID` suppose to be the address of the proxy. If this failed, it may mean that the controller service down.

Moreover, to let the device controller know whether the proxy service alive. The proxy will also set the value of key: `$PROXY_ADDRESS` to the `$DEVICE_CONTROLLER_ERVICE_ID` periodically.
