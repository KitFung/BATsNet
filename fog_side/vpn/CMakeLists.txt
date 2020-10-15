cmake_minimum_required( VERSION 3.9 )
project(vpn)

install(DIRECTORY DESTINATION /opt/aiot/vpn_service)
install(FILES report_ip.py DESTINATION /opt/aiot/vpn_service)
install(
    FILES vpn_tunnel.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION /opt/aiot/vpn_service
)

install(FILES systemd/vpn_tunnel.service DESTINATION /etc/systemd/system)
