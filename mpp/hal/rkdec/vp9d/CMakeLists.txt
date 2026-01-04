# vim: syntax=cmake

# hal vp9 decoder sourse
set(HAL_VP9D_SRC
    hal_vp9d_com.c
    )

add_hal_source(HAL_VP9D_SRC HAVE_VDPU341 hal_vp9d_rkv.c)
add_hal_source(HAL_VP9D_SRC HAVE_VDPU34X hal_vp9d_vdpu34x.c)
add_hal_source(HAL_VP9D_SRC HAVE_VDPU382 hal_vp9d_vdpu382.c)
add_hal_source(HAL_VP9D_SRC HAVE_VDPU383 hal_vp9d_vdpu383.c)
add_hal_source(HAL_VP9D_SRC HAVE_VDPU384B hal_vp9d_vdpu384b.c)

if (HAL_VP9D_SRC)
    add_library(hal_vp9d_api OBJECT ${HAL_VP9D_SRC})
endif()
