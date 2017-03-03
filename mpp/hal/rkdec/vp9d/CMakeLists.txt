# vim: syntax=cmake

# hal vp9 decoder api
set(HAL_VP9D_API
    ../../inc/hal_vp9d_api.h
    )

# hal vp9 header
set(HAL_VP9D_HDR
    )

# hal vp9 decoder sourse
set(HAL_VP9D_SRC
    hal_vp9d_api.c
    )

add_library(hal_vp9d STATIC
    ${HAL_VP9D_API}
    ${HAL_VP9D_HDR}
    ${HAL_VP9D_SRC}
    )

target_link_libraries(hal_vp9d mpp_base)
set_target_properties(hal_vp9d PROPERTIES FOLDER "mpp/hal")

