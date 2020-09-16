#!/bin/bash
export DEVICE_SUBNET=192.168.100.0/24
export DEVICE_IP=192.168.100.3
export ETH_SUBNET=10.0.0.0/16
export ETH_NET_DEV=enp5s0
export ENCODED_PROTO=225
export BR_DEV=br0

export IP_FOG1=10.0.0.2
export PORT_RANGE_FOG1=10000:12000
export FOG1_E_QUEUE=201
export FOG1_D_QUEUE=202

export ENCODE_RANGE=10000:12000


iptables -P INPUT ACCEPT
iptables -P FORWARD ACCEPT
iptables -P OUTPUT ACCEPT
iptables -t nat -F
iptables -t mangle -F
iptables -F
iptables -X

# sleep 5
echo 1 > /proc/sys/net/ipv4/ip_forward
# route add 172.16.19.11 gw 192.168.100.1 dev br0
# route add -net 10.32.0.0/16 gw 192.168.100.3 dev br0
# route add -net 172.17.19.0/27 gw 10.32.0.254 dev enp5s0
# route add -net 172.16.19.0/27 gw 10.32.0.254 dev enp5s0

iptables -A OUTPUT -j ACCEPT -o $ETH_NET_DEV
# iptables -A INPUT -j ACCEPT -i lo -p tcp --dport $PORT_RANGE_FOG1
# iptables -A INPUT -j ACCEPT -i lo -p udp --dport $PORT_RANGE_FOG1

# iptables -t nat -I PREROUTING 1 -p $ENCODED_PROTO -j NFQUEUE --queue-num $FOG1_D_QUEUE
# iptables -t nat -I PREROUTING 2 -p $ENCODED_PROTO -j DNAT --to-destination $IP_FOG1
# iptables -t nat -I PREROUTING 2 -d $DEVICE_IP -p $ENCODED_PROTO -j NFQUEUE --queue-num $FOG1_D_QUEUE
# iptables -t nat -I PREROUTING 3 -d $DEVICE_IP -p tcp --dport $PORT_RANGE_FOG1 -j DNAT --to-destination $IP_FOG1
# iptables -t nat -I PREROUTING 4 -d $DEVICE_IP -p udp --dport $PORT_RANGE_FOG1 -j DNAT --to-destination $IP_FOG1

# Change the incoming forward packet dest ip to be fog ip
# iptables -t nat -I PREROUTING 1 -i $BR_DEV -p $ENCODED_PROTO -d $DEVICE_IP -j NFQUEUE --queue-num $FOG1_D_QUEUE
# iptables -t nat -I PREROUTING 1 -i $BR_DEV -p $ENCODED_PROTO -j DNAT --to-destination $IP_FOG1
# iptables -t nat -I PREROUTING 2 -i $BR_DEV -p tcp --dport $PORT_RANGE_FOG1 -j DNAT --to-destination $IP_FOG1
# iptables -t nat -I PREROUTING 3 -i $BR_DEV -p udp --dport $PORT_RANGE_FOG1 -j DNAT --to-destination $IP_FOG1

# iptables -t nat -I PREROUTING -j DNAT --to-destination $IP_FOG1
# Change the outgoing forward packet source ip to be this ip
# iptables -t nat -A POSTROUTING -o $BR_DEV -j MASQUERADE
iptables -t nat -A POSTROUTING -o $BR_DEV -j SNAT -s $IP_FOG1 --to $DEVICE_IP
# iptables -t nat -A POSTROUTING -o enp5s0 -j SNAT --to $DEVICE_IP
# iptables -t nat -A POSTROUTING -o $BR_DEV -j SNAT --to $DEVICE_IP

iptables -A FORWARD -p ospf -j DROP
iptables -A OUTPUT -p ospf -j ACCEPT
m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -s $DEVICE_SUBNET -d 192.168.100.7
iptables -A FORWARD -j NFQUEUE --queue-num 8 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -s $DEVICE_SUBNET -d 192.168.100.8
iptables -A FORWARD -j NFQUEUE --queue-num 9 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -s $DEVICE_SUBNET -d 192.168.100.9
iptables -A FORWARD -j NFQUEUE --queue-num 10 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -s $DEVICE_SUBNET -d 192.168.100.10
iptables -A FORWARD -j NFQUEUE --queue-num 11 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -s $DEVICE_SUBNET -d 192.168.100.11
iptables -A FORWARD -j NFQUEUE --queue-num 12 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -s $DEVICE_SUBNET -d 192.168.100.12
iptables -A FORWARD -j NFQUEUE --queue-num 13 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -s $DEVICE_SUBNET -d 192.168.100.13
iptables -A FORWARD -j NFQUEUE --queue-num 14 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -s $DEVICE_SUBNET -d 192.168.100.14
iptables -A FORWARD -j NFQUEUE --queue-num 53 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.3
iptables -A FORWARD -j NFQUEUE --queue-num 54 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.4
iptables -A FORWARD -j NFQUEUE --queue-num 55 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.5
iptables -A FORWARD -j NFQUEUE --queue-num 56 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.6
iptables -A FORWARD -j NFQUEUE --queue-num 57 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.7
iptables -A FORWARD -j NFQUEUE --queue-num 58 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.8
iptables -A FORWARD -j NFQUEUE --queue-num 59 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.9
iptables -A FORWARD -j NFQUEUE --queue-num 60 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.10
iptables -A FORWARD -j NFQUEUE --queue-num 61 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.11
iptables -A FORWARD -j NFQUEUE --queue-num 62 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.12
iptables -A FORWARD -j NFQUEUE --queue-num 63 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.13
iptables -A FORWARD -j NFQUEUE --queue-num 64 -m physdev --physdev-is-bridged -p $ENCODED_PROTO ! -d $DEVICE_SUBNET -s 192.168.100.14

# iptables -A FORWARD -j NFQUEUE --queue-num $FOG1_E_QUEUE ! -p $ENCODED_PROTO -d $IP_FOG1 --dports $PORT_RANGE_FOG1 ! -s $DEVICE_SUBNET
iptables -A FORWARD -j NFQUEUE --queue-num $FOG1_E_QUEUE -s $IP_FOG1 -d $DEVICE_SUBNET
# iptables -A FORWARD -j NFQUEUE --queue-num $FOG1_D_QUEUE -p $ENCODED_PROTO -d $IP_FOG1 -s $DEVICE_SUBNET

# Avoid encode some control or discover package
iptables -A OUTPUT -j NFQUEUE --queue-num 102 -p tcp --dport $ENCODE_RANGE -d $DEVICE_SUBNET -s $DEVICE_IP
iptables -A OUTPUT -j NFQUEUE --queue-num 102 -p udp --dport $ENCODE_RANGE -d $DEVICE_SUBNET -s $DEVICE_IP

iptables -A INPUT -j NFQUEUE --queue-num 152 -p $ENCODED_PROTO -s $DEVICE_SUBNET -d $DEVICE_IP

cd /home/nclab/ip
./fork e $FOG1_E_QUEUE d $FOG1_D_QUEUE \
    e 102 d 152 r 3 r 53 r 4 r 54 r 5 r 55 r 6 r 56 r 7 r 57 r 8 r 58 r 9 r 59 r 10 r 60 r 11 r 61 r 12 r 62 r 13 r 63 r 14 r 64
