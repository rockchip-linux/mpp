# vim: syntax=cmake
include_directories(.)

add_library(${CODEC_JPEGE} STATIC
    jpege_api_v2.c
    )

target_link_libraries(${CODEC_JPEGE} mpp_base)
set_target_properties(${CODEC_JPEGE} PROPERTIES FOLDER "mpp/codec")
