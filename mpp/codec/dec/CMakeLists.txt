# vim: syntax=cmake

add_subdirectory(dummy)

if( HAVE_AVSD )
    add_subdirectory(avs)
endif()

if( HAVE_H263D )
    add_subdirectory(h263)
endif()

if( HAVE_H264D )
    add_subdirectory(h264)
endif()

if( HAVE_H265D )
    add_subdirectory(h265)
endif()

if( HAVE_MPEG2D )
    add_subdirectory(m2v)
endif()

if( HAVE_MPEG4D )
    add_subdirectory(mpg4)
endif()

if( HAVE_VP8D )
    add_subdirectory(vp8)
endif()

if( HAVE_VP9D )
    add_subdirectory(vp9)
endif()

if( HAVE_JPEGD )
    add_subdirectory(jpeg)
endif()
