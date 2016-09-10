# vim: syntax=cmake

if( HAVE_H264D )
    add_subdirectory(h264d)
endif()

if( HAVE_H265D )
    add_subdirectory(h265d)
endif()

if( HAVE_VP9D )
    add_subdirectory(vp9d)
endif()
