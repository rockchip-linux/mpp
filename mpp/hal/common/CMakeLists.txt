# vim: syntax=cmake
include_directories(.)

add_library(hal_common STATIC
    mpp_enc_refs.c
    )

set_target_properties(hal_common PROPERTIES FOLDER "mpp/hal/common")

add_subdirectory(h264)
