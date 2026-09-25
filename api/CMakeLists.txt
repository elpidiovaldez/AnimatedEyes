# Optional C++ API for controlling AnimatedEyes from other programs. Use it from your own
# CMake project with:
#
#   add_subdirectory(/path/to/AnimatedEyes/api animatedeyes_api)
#   target_link_libraries(your_program PRIVATE AnimatedEyes::api)
#
# then #include <AnimatedEyes/Eyes.hpp>.

cmake_minimum_required(VERSION 3.16)

project(AnimatedEyesApi LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Use the Master client library the including project already has (e.g. BDX's), so it isn't
# linked twice. Otherwise build the copy in this repository.
if(TARGET MasterClient::masterclient)
    set(ANIMATEDEYES_MASTERCLIENT MasterClient::masterclient)
elseif(TARGET masterclient)
    set(ANIMATEDEYES_MASTERCLIENT masterclient)
else()
    add_library(masterclient STATIC ${CMAKE_CURRENT_LIST_DIR}/../master/client/src/MasterClient.cpp)
    target_include_directories(masterclient PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../master/client/include ${ZMQ_INCLUDE_DIRS})
    target_link_libraries(masterclient PUBLIC ${ZMQ_LIBRARIES})
    target_compile_features(masterclient PUBLIC cxx_std_20)
    set(ANIMATEDEYES_MASTERCLIENT masterclient)
endif()

add_library(animatedeyes_api INTERFACE)
add_library(AnimatedEyes::api ALIAS animatedeyes_api)
target_include_directories(animatedeyes_api INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include ${ZMQ_INCLUDE_DIRS})
target_link_libraries(animatedeyes_api INTERFACE ${ANIMATEDEYES_MASTERCLIENT} ${ZMQ_LIBRARIES})
target_compile_features(animatedeyes_api INTERFACE cxx_std_20)

# Build the example when this directory is built on its own
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_executable(eyes_example example/eyes_example.cpp)
    target_link_libraries(eyes_example PRIVATE AnimatedEyes::api)
endif()
