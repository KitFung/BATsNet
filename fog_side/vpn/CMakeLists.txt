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

set(SYSTEM_RELOAD_CMD "execute_process(COMMAND /bin/systemctl daemon-reload)")
install(CODE ${SYSTEM_RELOAD_CMD})

set(VPN_RESTART_CMD "execute_process(COMMAND /usr/sbin/service vpn_tunnel restart)")
install(CODE ${VPN_RESTART_CMD})

set(VPN_ENABLE_CMD "execute_process(COMMAND /bin/systemctl enable vpn_tunnel)")
install(CODE ${VPN_ENABLE_CMD})
