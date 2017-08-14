# vim: syntax=cmake

add_subdirectory(dummy)

if(HAVE_H264E)
add_subdirectory(h264)
endif()

if(HAVE_JPEGE)
add_subdirectory(jpeg)
endif()

if(HAVE_H265E)
add_subdirectory(h265)
endif()

if(HAVE_VP8E)
add_subdirectory(vp8)
endif()

