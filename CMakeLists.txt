cmake_minimum_required(VERSION 3.16)

project(AnimatedEyes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

add_executable(AnimatedEyes
    src/main.cpp
    master/client/src/MasterClient.cpp

    src/eyes/240x240/disp_240_120.cpp
    src/eyes/240x240/disp_240_125.cpp
    src/eyes/240x240/disp_240_130.cpp
    src/eyes/240x240/noeyelids_120.cpp
    src/eyes/240x240/polarAngle_240.cpp

    src/eyes/240x240/polarDist_240_120_70_0.cpp
    src/eyes/240x240/polarDist_240_120_75_0.cpp
    src/eyes/240x240/polarDist_240_120_115_0.cpp

    src/eyes/240x240/polarDist_240_125_50_0.cpp
    src/eyes/240x240/polarDist_240_125_60_0.cpp
    src/eyes/240x240/polarDist_240_125_75_0.cpp
    src/eyes/240x240/polarDist_240_125_85_0.cpp
    src/eyes/240x240/polarDist_240_125_90_90.cpp
    src/eyes/240x240/polarDist_240_125_100_0.cpp
    src/eyes/240x240/polarDist_240_125_110_100.cpp
    src/eyes/240x240/polarDist_240_125_125_0.cpp

    src/eyes/240x240/polarDist_240_130_95_0.cpp
    src/eyes/240x240/polarDist_240_130_115_0.cpp
)

#target_compile_options(AnimatedEyes PRIVATE    -Wall    -Wextra    -g    -O0    -fno-omit-frame-pointer)

target_include_directories(AnimatedEyes PRIVATE
    src
    src/displays
    src/eyes/240x240
    src/GC9A01
    src/sensors
    master/client/include
    ${ZMQ_INCLUDE_DIRS}
)

target_link_libraries(AnimatedEyes PRIVATE
    rpidisplaygl
    lgpio
    ${ZMQ_LIBRARIES}
)

# Tests for the command and message parsing (no displays needed): ctest --test-dir build
enable_testing()
add_executable(eyes_tests tests/test_parsing.cpp)
target_include_directories(eyes_tests PRIVATE src)
add_test(NAME parsing COMMAND eyes_tests)
