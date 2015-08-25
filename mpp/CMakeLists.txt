# vim: syntax=cmake
include_directories(.)
include_directories(codec/inc)
include_directories(hal/inc)


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
add_library(mpp STATIC
    mpp_info.cpp
    mpp_buffer_impl.cpp
    mpp_buffer.cpp
    mpp_packet.cpp
    mpp_frame.cpp
    mpp.cpp
    mpi_impl.cpp
    mpi.cpp
    )

add_subdirectory(legacy)

add_subdirectory(test)

target_link_libraries(mpp mpp_legacy osal)
