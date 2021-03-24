# vim: syntax=cmake
include_directories(.)

# av1 decoder api
set(AV1_D_API
    ../../inc/av1d_api.h
    )

# AV1 decoder header
set(AV1D_HDR
    )

# av1 decoder sourse
set(AV1D_SRC
    av1d_api.c
    av1d_parser.c
    av1d_parser2_syntax.c
    av1d_cbs.c
    av1d_probs.c
    av1_entropymode.c
    )

add_library(${CODEC_AV1D} STATIC
    ${AV1D_API}
    ${AV1D_HDR}
    ${AV1D_SRC}
    )

target_link_libraries(${CODEC_AV1D} mpp_base)
set_target_properties(${CODEC_AV1D} PROPERTIES FOLDER "mpp/codec")
