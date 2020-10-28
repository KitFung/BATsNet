cmake_minimum_required(VERSION 3.9)
project(velodyne_driver VERSION 0.0.1 LANGUAGES C CXX)

if(INSTALL_DRIVER)
  # Set minimum C++ standard to C++11
  if (NOT "${CMAKE_CXX_STANDARD_COMPUTED_DEFAULT}")
    message(STATUS "Changing CXX_STANDARD from C++98 to C++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  elseif ("${CMAKE_CXX_STANDARD_COMPUTED_DEFAULT}" STREQUAL "98")
    message(STATUS "Changing CXX_STANDARD from C++98 to C++11")
    set(CMAKE_CXX_STANDARD 11)
  endif()

  # # This driver uses Boost threads
  # find_package(Boost REQUIRED COMPONENTS thread)

  # libpcap provides no pkg-config or find_package module:
  set(libpcap_LIBRARIES -lpcap)

  include_directories(include ${Boost_INCLUDE_DIR})

  # compile the driver and input library
  add_subdirectory(src/lib)
  add_subdirectory(src/driver)
endif(INSTALL_DRIVER)
