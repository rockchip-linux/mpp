include_directories(.)

set(VP8E_API
    )
set(VP8E_COMMON
    )
set(VP8E_HDR
    vp8e_debug.h
    vp8e_rc.h)
set(VP8E_SRC
    vp8e_api.c
    vp8e_rc.c)

add_library(codec_vp8e STATIC
            ${VP8E_API}
            ${VP8E_COMMON}
            ${VP8E_HDR}
            ${VP8E_SRC}
            )

target_link_libraries(codec_vp8e mpp_base)
set_target_properties(codec_vp8e PROPERTIES FOLDER "mpp/codec")
