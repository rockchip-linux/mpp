# vim: syntax=cmake
cmake_minimum_required(VERSION 2.6)
find_package(PkgConfig)

PROJECT(rockchip_mpp C CXX)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(LIBROCKCHIP_MPP_VERSION "1")
set(LIBROCKCHIP_MPP_ABI_VERSION "1")
# ----------------------------------------------------------------------------
# setup mpp codec config first
# ----------------------------------------------------------------------------
include(codecs.cmake)

# ----------------------------------------------------------------------------
# add include directory
# ----------------------------------------------------------------------------
include_directories(inc)
include_directories(common)
include_directories(base/inc)
include_directories(codec/inc)
include_directories(hal/inc)
include_directories(hal/common)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
)

set_target_properties(rockchip_mpp PROPERTIES VERSION
    ${LIBROCKCHIP_MPP_VERSION} SOVERSION ${LIBROCKCHIP_MPP_ABI_VERSION})

install(TARGETS rockchip_mpp LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(TARGETS rockchip_mpp_static ARCHIVE DESTINATION
    "${CMAKE_INSTALL_LIBDIR}")

add_subdirectory(legacy)

add_subdirectory(test)
