# vim: syntax=cmake

# hal h264 decoder sourse
set(HAL_H264D_SRC
    hal_h264d_api.c
    hal_h264d_com.c
    hal_h264d_vdpu34x.c
    hal_h264d_vdpu382.c
    hal_h264d_vdpu383.c
    hal_h264d_vdpu384a.c
    hal_h264d_rkv_reg.c
    hal_h264d_vdpu2.c
    hal_h264d_vdpu1.c
    hal_h264d_vdpu_com.c
    )

add_library(${HAL_H264D} OBJECT ${HAL_H264D_SRC})

target_link_libraries(${HAL_H264D})
set_target_properties(${HAL_H264D} PROPERTIES FOLDER "mpp/hal")

