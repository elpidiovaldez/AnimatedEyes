cmake_minimum_required(VERSION 3.16)

project(EndpointMaster)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(yaml-cpp REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

add_executable(master
    Master.cpp
)

target_include_directories(master PRIVATE
    ${ZMQ_INCLUDE_DIRS}
)

target_link_libraries(master PRIVATE
    yaml-cpp
    ${ZMQ_LIBRARIES}
)
