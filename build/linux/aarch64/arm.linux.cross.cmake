
cmake_minimum_required( VERSION 2.6.3 )

if(NOT TOOLCHAIN)
    set(TOOLCHAIN "aarch64-linux-gnu-")
endif()

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "${TOOLCHAIN}gcc")
SET(CMAKE_CXX_COMPILER "${TOOLCHAIN}g++")
#SET(CMAKE_SYSTEM_PROCESSOR "armv7-a")
SET(CMAKE_SYSTEM_PROCESSOR "armv8-a")

add_definitions(-fPIC)
add_definitions(-DARMLINUX)
add_definitions(-Dlinux)
