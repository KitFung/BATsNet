cmake_minimum_required( VERSION 3.9 )
project( data_collector VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# find_package(Protobuf REQUIRED)

# include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS EXPORT_MACRO DLL_EXPORT foo.proto)

add_library(data_collector SHARED src/data_collector.cc)
set_target_properties(data_collector PROPERTIES VERSION ${PROJECT_VERSION})
# target_link_libraries(data_collector ${Protobuf_LIBRARIES})
target_link_libraries(data_collector mosquitto mosquittopp)
target_compile_options (data_collector PRIVATE -Wno-deprecated-declarations)