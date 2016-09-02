# vim: syntax=cmake
include_directories(.)

add_library(codec_jpege STATIC
    jpege_api.c
    )

target_link_libraries(codec_jpege mpp_base)
set_target_properties(codec_jpege PROPERTIES FOLDER "mpp/codec")
