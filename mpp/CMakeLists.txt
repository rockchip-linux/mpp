# vim: syntax=cmake
cmake_minimum_required(VERSION 2.6)
find_package(PkgConfig)

PROJECT(rockchip_mpp C CXX)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
# ----------------------------------------------------------------------------
# setup mpp codec config first
# ----------------------------------------------------------------------------
include(codecs.cmake)

# ----------------------------------------------------------------------------
# add include directory
# ----------------------------------------------------------------------------
include_directories(common)
include_directories(base/inc)
include_directories(codec/inc)
include_directories(hal/inc)
include_directories(hal/common)

# external libraries


# ----------------------------------------------------------------------------
# add mpp base component
# ----------------------------------------------------------------------------
add_subdirectory(base)

# ----------------------------------------------------------------------------
# add codec parsers
# ----------------------------------------------------------------------------
add_subdirectory(codec)

# ----------------------------------------------------------------------------
# add register generation hal
# ----------------------------------------------------------------------------
add_subdirectory(hal)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
add_library(rockchip_mpp SHARED
    mpp_info.cpp
    mpp.cpp
    mpi_impl.cpp
    mpi.cpp
)
target_link_libraries(rockchip_mpp
    mpp_base mpp_codec mpp_hal
)
target_include_directories(rockchip_mpp PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_library(rockchip_mpp_static STATIC
    mpp_info.cpp
    mpp.cpp
    mpi_impl.cpp
    mpi.cpp
)
target_link_libraries(rockchip_mpp_static
    mpp_base mpp_codec mpp_hal
)
target_include_directories(rockchip_mpp_static PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
add_subdirectory(legacy)

add_subdirectory(test)

install(TARGETS mpp mpp_shared
        ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
        LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
        )
