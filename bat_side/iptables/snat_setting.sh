iptables -t nat -A POSTROUTING -o br0 \
    -s 10.42.0.0/24 \
    ! -d 10.42.0.0/24 \
    -j SNAT --to 192.168.100.3
