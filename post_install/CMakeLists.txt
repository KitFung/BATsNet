set(CHMOD_CMD "execute_process(COMMAND /bin/chmod -R 0777 ${INSTALL_BASE_DIR})")
install(CODE ${CHMOD_CMD})
