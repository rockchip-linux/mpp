# vim: syntax=cmake
include_directories(.)

# ----------------------------------------------------------------------------
# add hal common file
# ----------------------------------------------------------------------------
add_library(hal_common OBJECT
    hal_info.c
    hal_bufs.c
    )

if( HAVE_H264E )
    add_subdirectory(h264)
endif()

if( HAVE_H265E )
    add_subdirectory(h265)
endif()

if( HAVE_JPEGE )
    add_subdirectory(jpeg)
endif()

if( HAVE_AV1D )
    add_subdirectory(av1)
endif()
