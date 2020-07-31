cmake_minimum_required( VERSION 3.9 )
project(transport)

include(ExternalProject)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

ExternalProject_Add(cpp-ipc
    GIT_REPOSITORY https://github.com/mutouyun/cpp-ipc
    PREFIX ${EXTERNAL_INSTALL_LOCATION}
    SOURCE_SUBDIR build
    INSTALL_COMMAND cmake -E echo "Skipping install step."
)

ExternalProject_Get_property(cpp-ipc SOURCE_DIR)

include_directories(${SOURCE_DIR}/include)
add_library( ipc SHARED IMPORTED )
set_target_properties( ipc PROPERTIES IMPORTED_LOCATION ${SOURCE_DIR}/output/libipc.so )

add_executable(transport src/chat_test.cc)
add_dependencies(transport cpp-ipc)
target_link_libraries(transport ipc pthread rt)

# add_executable(transport main.cpp)
# add_dependencies(FirstTest cpp-ipc)
# target_link_libraries(FirstTest cpp-ipc)