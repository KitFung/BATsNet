import etcd3
import time
from python_hosts import Hosts, HostsEntry


# SERVER_IP = '127.0.0.1'
SERVER_IP = '137.189.97.26'

fog_map = {}

hosts = Hosts(path='/etc/hosts')


def update_host():
    nodes_address = []
    for entry in hosts.entries:
        if entry.names is not None:
            for name in entry.names:
                if name.startswith('node'):
                    nodes_address.append(entry.address)
                    break
    for addr in nodes_address:
        hosts.remove_all_matching(address=addr)
    for name, ip in fog_map.items():
        name = name.decode()
        name = name.strip('/').replace('/', '.')
        ip = ip.decode()
        new_entry = HostsEntry(
            entry_type='ipv4',
            address=ip,
            names=[name])
        hosts.add([new_entry])
    hosts.write()


def watch_callback(event):
    # print(event)
    # print(event.events)
    for e in event.events:
        if isinstance(e, etcd3.events.PutEvent):
            fog_map[e.key] = etcd.get(e.key)[0]
        elif isinstance(e, etcd3.events.DeleteEvent):
            if e.key in fog_map:
                del fog_map[e.key]
    # print(fog_map)
    update_host()


etcd = etcd3.client(host=SERVER_IP, port=2379)
watch_id = etcd.add_watch_prefix_callback(
    "/node/", watch_callback)

while True:
    time.sleep(1)
# # cancel watch
# etcd.cancel_watch(watch_id)
