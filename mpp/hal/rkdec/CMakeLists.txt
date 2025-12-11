# vim: syntax=cmake
include_directories(inc)

add_subdirectory(common)

if( HAVE_AVSD )
    add_subdirectory(avsd)
endif()

if( HAVE_AVS2D )
    add_subdirectory(avs2d)
endif()

if( HAVE_H264D )
    add_subdirectory(h264d)
endif()

if( HAVE_H265D )
    add_subdirectory(h265d)
endif()

if( HAVE_VP9D )
    add_subdirectory(vp9d)
endif()

if( HAVE_AV1D )
    add_subdirectory(av1d)
endif()
