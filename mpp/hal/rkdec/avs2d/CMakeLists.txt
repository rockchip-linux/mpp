# vim: syntax=cmake

# hal avs2 decoder api
set(HAL_AVS2D_API
    )

# hal avs2 header
set(HAL_AVS2D_HDR
    hal_avs2d_global.h
    hal_avs2d_rkv.h
    )

# hal avs2 decoder sourse
set(HAL_AVS2D_SRC
    hal_avs2d_rkv.c
    hal_avs2d_api.c
    )

add_library(hal_avs2d STATIC
            ${HAL_AVS2D_API}
            ${HAL_AVS2D_HDR}
            ${HAL_AVS2D_SRC}
            )

target_link_libraries(hal_avs2d mpp_base)
set_target_properties(hal_avs2d PROPERTIES FOLDER "mpp/hal")

