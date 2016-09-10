# vim: syntax=cmake
include_directories(.)

# vp9 decoder api
set(VP9_D_API
    ../../inc/vp9d_api.h
    )

# vp9 decoder header
set(VP9D_HDR
    )

# vp9 decoder sourse
set(VP9D_SRC
    vp9d_api.c
    vp9d_parser.c
    vpx_rac.c
    vp9d_parser2_syntax.c
    )

add_library(${CODEC_VP9D} STATIC
    ${VP9D_API}
    ${VP9D_HDR}
    ${VP9D_SRC}
    )

target_link_libraries(${CODEC_VP9D} mpp_base)
set_target_properties(${CODEC_VP9D} PROPERTIES FOLDER "mpp/codec")
