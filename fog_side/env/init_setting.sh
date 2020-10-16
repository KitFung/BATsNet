source /etc/profile.d/fog_env.sh

route add -net 192.168.100.0/24 gw $BAT_IP dev $BAT_NIC
