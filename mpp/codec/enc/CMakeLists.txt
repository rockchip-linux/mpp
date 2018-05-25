# vim: syntax=cmake

add_subdirectory(dummy)

if(HAVE_H264E)
add_subdirectory(h264)
endif()

if(HAVE_JPEGE)
add_subdirectory(jpeg)
endif()

