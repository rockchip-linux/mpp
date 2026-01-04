# vim: syntax=cmake
include_directories(.)

# hal vp8 decoder sourse
set(HAL_VP8D_SRC)

add_hal_source(HAL_VP8D_SRC HAVE_VDPU1 hal_vp8d_vdpu1.c)
add_hal_source(HAL_VP8D_SRC HAVE_VDPU2 hal_vp8d_vdpu2.c)

if (HAL_VP8D_SRC)
    add_library(hal_vp8d_api OBJECT ${HAL_VP8D_SRC})
endif()