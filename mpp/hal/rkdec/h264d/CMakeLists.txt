# vim: syntax=cmake

# hal h264 decoder sourse
set(HAL_H264D_SRC
    hal_h264d_api.c
    hal_h264d_com.c
    hal_h264d_vdpu_com.c
    )

add_hal_source(HAL_H264D_SRC HAVE_VDPU2     hal_h264d_vdpu2.c)
add_hal_source(HAL_H264D_SRC HAVE_VDPU1     hal_h264d_vdpu1.c)
add_hal_source(HAL_H264D_SRC HAVE_VDPU341   hal_h264d_rkv_reg.c)
add_hal_source(HAL_H264D_SRC HAVE_VDPU34X   hal_h264d_vdpu34x.c)
add_hal_source(HAL_H264D_SRC HAVE_VDPU382   hal_h264d_vdpu382.c)
add_hal_source(HAL_H264D_SRC HAVE_VDPU383   hal_h264d_vdpu383.c)
add_hal_source(HAL_H264D_SRC HAVE_VDPU384A  hal_h264d_vdpu384a.c)
add_hal_source(HAL_H264D_SRC HAVE_VDPU384B  hal_h264d_vdpu384b.c)

if (HAL_H264D_SRC)
    add_library(hal_h264d_api OBJECT ${HAL_H264D_SRC})
endif()
