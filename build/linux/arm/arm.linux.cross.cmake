
cmake_minimum_required( VERSION 2.6.3 )

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "arm-linux-gcc")
SET(CMAKE_CXX_COMPILER "arm-linux-g++")
#SET(CMAKE_SYSTEM_PROCESSOR "armv7-a")
SET(CMAKE_SYSTEM_PROCESSOR "armv7-a_hardfp")
set(ARMLINUX true)
add_definitions(-fPIC)
add_definitions(-DARMLINUX)
include_directories(osal)

