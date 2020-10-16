install(FILES fog_env.sh DESTINATION /etc/profile.d)

install(DIRECTORY DESTINATION ${INSTALL_BASE_DIR}/env)
install(
    FILES init_setting.py
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${INSTALL_BASE_DIR}/env
)

install(FILES systemd/fog_env.service DESTINATION /etc/systemd/system)

# install(CODE "execute_process(COMMAND set_route.sh)")
