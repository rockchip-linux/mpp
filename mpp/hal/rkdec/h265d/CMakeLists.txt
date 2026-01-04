# vim: syntax=cmake
# hal h265 reg

set(HAL_H265D_SRC
    hal_h265d_com.c
    )

add_hal_source(HAL_H265D_SRC HAVE_VDPU341 hal_h265d_rkv.c)
add_hal_source(HAL_H265D_SRC HAVE_VDPU34X hal_h265d_vdpu34x.c)
add_hal_source(HAL_H265D_SRC HAVE_VDPU382 hal_h265d_vdpu382.c)
add_hal_source(HAL_H265D_SRC HAVE_VDPU383 hal_h265d_vdpu383.c)
add_hal_source(HAL_H265D_SRC HAVE_VDPU384A hal_h265d_vdpu384a.c)
add_hal_source(HAL_H265D_SRC HAVE_VDPU384B hal_h265d_vdpu384b.c)

if (HAL_H265D_SRC)
    add_library(hal_h265d_api OBJECT ${HAL_H265D_SRC})
endif()
