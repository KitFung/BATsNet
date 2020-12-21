### IPC

Transmitting data through shared memory. Only support 1 to 1 transmission right now.

## BlockChannel

Used to transfer block data between two computer. The underlying implementation is MQTT.

### binding

Provide a python binding for transport module. Support two mode: 1. IPC, 2. BlockChannel while the support data type is raw buffer.
