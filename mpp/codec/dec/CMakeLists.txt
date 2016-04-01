# vim: syntax=cmake

add_subdirectory(dummy)

add_subdirectory(h264)

add_subdirectory(h265)

add_subdirectory(vp9)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/avs_impl/")
add_subdirectory(avs_impl)
endif()

add_subdirectory(avs)



