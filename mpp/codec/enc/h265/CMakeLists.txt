# vim: syntax=cmake
include_directories(.)

add_library(${CODEC_H265E} STATIC
    h265e_api.c
    )

target_link_libraries(${CODEC_H265E} mpp_base)
set_target_properties(${CODEC_H265E} PROPERTIES FOLDER "mpp/codec")
