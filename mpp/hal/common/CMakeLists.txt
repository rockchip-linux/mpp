# vim: syntax=cmake
include_directories(.)

# ----------------------------------------------------------------------------
# add hal common file
# ----------------------------------------------------------------------------
add_library(hal_common STATIC
    hal_info.c
    hal_bufs.c
    )

target_link_libraries(hal_common mpp_base)
set_target_properties(hal_common PROPERTIES FOLDER "mpp/hal/common")

add_subdirectory(h264)
add_subdirectory(h265)
