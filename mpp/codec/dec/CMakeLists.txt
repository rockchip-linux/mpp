# vim: syntax=cmake

add_subdirectory(dummy)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/avs")
    set(HAVE_AVSD true)
    set(CODEC_AVSD codec_avsd)
    add_definitions(-DHAVE_AVSD)
    add_subdirectory(avs)
endif()

add_subdirectory(h263)

add_subdirectory(h264)

add_subdirectory(h265)

add_subdirectory(m2v)

add_subdirectory(mpg4)

add_subdirectory(vp8)

add_subdirectory(vp9)

add_subdirectory(jpeg)
