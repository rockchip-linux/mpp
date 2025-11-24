# vim: syntax=cmake
add_subdirectory(common)

if( HAVE_H264E )
    add_subdirectory(h264e)
endif()

if( HAVE_H265E )
    add_subdirectory(h265e)
endif()

if( HAVE_JPEGE )
    add_subdirectory(jpege)
endif()
