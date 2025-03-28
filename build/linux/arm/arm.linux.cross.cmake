SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_COMPILER "${TOOLCHAIN}gcc")
SET(CMAKE_CXX_COMPILER "${TOOLCHAIN}g++")
SET(CMAKE_SYSTEM_PROCESSOR "armv7-a")
#SET(CMAKE_SYSTEM_PROCESSOR "armv7-a_hardfp")

add_definitions(-fPIC)
add_definitions(-DARMLINUX)
add_definitions(-Dlinux)
