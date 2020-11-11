# vim: syntax=cmake
include_directories(.)

add_library(${CODEC_H265E} STATIC
    h265e_api.c
    h265e_dpb.c
    h265e_ps.c
    h265e_slice.c
    h265e_header_gen.c
    h265e_stream.c
    h265e_syntax.c
    h265e_enctropy.c
    )

target_link_libraries(${CODEC_H265E} mpp_base)
set_target_properties(${CODEC_H265E} PROPERTIES FOLDER "mpp/codec")
