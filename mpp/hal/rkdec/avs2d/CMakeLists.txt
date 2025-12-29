# vim: syntax=cmake

# hal avs2 decoder sourse
set(HAL_AVS2D_SRC
    hal_avs2d_com.c
    hal_avs2d_api.c
    )

add_hal_source(HAL_AVS2D_SRC HAVE_VDPU381 hal_avs2d_rkv.c)
add_hal_source(HAL_AVS2D_SRC HAVE_VDPU382 hal_avs2d_vdpu382.c)
add_hal_source(HAL_AVS2D_SRC HAVE_VDPU383 hal_avs2d_vdpu383.c)
add_hal_source(HAL_AVS2D_SRC HAVE_VDPU384B hal_avs2d_vdpu384b.c)

if (HAL_AVS2D_SRC)
    add_library(hal_avs2d_api OBJECT ${HAL_AVS2D_SRC})
endif()
