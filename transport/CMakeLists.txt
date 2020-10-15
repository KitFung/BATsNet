cmake_minimum_required( VERSION 3.9 )
project(transport)

include(ExternalProject)
find_package(Protobuf REQUIRED)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)


EXEC_PROGRAM(protoc 
  ARGS -I=${CMAKE_CURRENT_SOURCE_DIR}/proto
       --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto
       ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

ExternalProject_Add(cpp-ipc
    GIT_REPOSITORY https://github.com/KitFung/cpp-ipc
    PREFIX ${EXTERNAL_INSTALL_LOCATION}
    SOURCE_SUBDIR build
    INSTALL_COMMAND cmake -E echo "Skipping install step."
)

ExternalProject_Get_property(cpp-ipc SOURCE_DIR)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${SOURCE_DIR}/..)

add_library( ipc SHARED IMPORTED )
set_target_properties( ipc PROPERTIES IMPORTED_LOCATION ${SOURCE_DIR}/output/libipc.so )

add_executable(chat_test src/chat_test.cc)
add_dependencies(chat_test cpp-ipc)
target_link_libraries(chat_test ipc pthread rt)

add_library(transport INTERFACE)
add_dependencies(transport cpp-ipc)
target_include_directories(transport INTERFACE include/ ${SOURCE_DIR}/..)
target_link_libraries(transport INTERFACE ipc pthread rt mosquitto mosquittopp service_discovery)
target_compile_options (transport INTERFACE -Wno-deprecated-declarations)

add_executable(ipc_test src/ipc_test.cc proto/simple.pb.cc)
target_link_libraries(ipc_test transport ${Protobuf_LIBRARIES})

add_executable(block_message_test src/block_message_test.cc proto/simple.pb.cc)
target_link_libraries(block_message_test transport ${Protobuf_LIBRARIES} service_discovery service_node)

add_subdirectory(binding/python)

# install(TARGETS service_discovery DESTINATION ${BATSNET_LIB_INSTALL_DIR})
