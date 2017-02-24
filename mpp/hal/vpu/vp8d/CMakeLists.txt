# vim: syntax=cmake
include_directories(.)

# hal vp8 decoder api
set(HAL_VP8D_API
    ../../inc/hal_vp8d_api.h
    ../../../common/vp8d_syntax.h
    )

# hal vp8 header
set(HAL_VP8D_HDR
    hal_vp8d_base.h
    )

# hal vp8 decoder sourse
set(HAL_VP8D_SRC
    hal_vp8d_api.c
    hal_vp8d_vdpu2.c
    hal_vp8d_vdpu1.c
    )

add_library(hal_vp8d STATIC
            ${HAL_VP8D_API}
            ${HAL_VP8D_HDR}
            ${HAL_VP8D_SRC}
            )

target_link_libraries(hal_vp8d mpp_base)
set_target_properties(hal_vp8d PROPERTIES FOLDER "mpp/hal")

