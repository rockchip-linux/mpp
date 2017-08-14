# vim: syntax=cmake

if( HAVE_H263D )
    add_subdirectory(h263d)
endif()

if( HAVE_MPEG2D )
    add_subdirectory(m2vd)
endif()

if( HAVE_MPEG4D )
    add_subdirectory(mpg4d)
endif()

if( HAVE_VP8D )
    add_subdirectory(vp8d)
endif()

if( HAVE_JPEGD )
    add_subdirectory(jpegd)
endif()

if( HAVE_JPEGE )
    add_subdirectory(jpege)
endif()

if( HAVE_H264E )
    add_subdirectory(h264e)
endif()

if( HAVE_VP8E)
    add_subdirectory(vp8e)
endif()