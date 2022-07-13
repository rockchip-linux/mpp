# vim: syntax=cmake
include_directories(.)

# avs2 decoder api
set(AVS2D_API

    )
set(AVS2D_COMMON

    )

# avs2 decoder header
set(AVS2D_HDR
    avs2d_global.h
    avs2d_parse.h
    avs2d_ps.h
    avs2d_dpb.h
    )

# avs2 decoder sourse
set(AVS2D_SRC
    avs2d_api.c
    avs2d_parse.c
    avs2d_ps.c
    avs2d_dpb.c
    )

add_library(codec_avs2d STATIC
    ${AVS2D_API}
    ${AVS2D_COMMON}
    ${AVS2D_HDR}
    ${AVS2D_SRC}
    )

target_link_libraries(codec_avs2d mpp_base)
set_target_properties(codec_avs2d PROPERTIES FOLDER "mpp/codec")