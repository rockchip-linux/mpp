# vim: syntax=cmake
include_directories(.)

#vp8 decoder api
set(VP8D_API
    ../../inc/vp8d_api.h
    )

#vp8 decoder header
set(VP8D_HDR
    vp8d_parser.h
    vp8d_codec.h
    )

#vp8 decoder source
set(VP8D_SRC
    vp8d_api.c
    vp8d_parser.c
    )

add_library(${CODEC_VP8D} STATIC
    ${VP8D_API}
    ${VP8D_HDR}
    ${VP8D_SRC}
    )

target_link_libraries(${CODEC_VP8D}  mpp_base)
set_target_properties(${CODEC_VP8D} PROPERTIES FOLDER "mpp/codec")
