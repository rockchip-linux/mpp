# vim: syntax=cmake
include_directories(.)
include_directories(common)
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

set_target_properties(mpp PROPERTIES FOLDER "mpp") 

add_subdirectory(legacy)

target_link_libraries(mpp mpp_codec mpp_hal osal)

add_subdirectory(test)

