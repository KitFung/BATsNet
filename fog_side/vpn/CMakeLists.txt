cmake_minimum_required( VERSION 3.9 )
project(vpn)

install(DIRECTORY DESTINATION ${INSTALL_BASE_DIR}/vpn_service)
install(FILES report_ip.py DESTINATION ${INSTALL_BASE_DIR}/vpn_service)
install(
    FILES vpn_tunnel.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${INSTALL_BASE_DIR}/vpn_service
)

install(FILES systemd/vpn_tunnel.service DESTINATION /etc/systemd/system)
