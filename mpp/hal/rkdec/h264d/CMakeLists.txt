# vim: syntax=cmake

# hal h264 decoder sourse
set(HAL_H264D_SRC
    hal_h264d_api.c
    hal_h264d_vdpu34x.c
    hal_h264d_rkv_reg.c
    hal_h264d_vdpu2.c
    hal_h264d_vdpu1.c
    hal_h264d_vdpu_com.c
    )

add_library(hal_h264d STATIC ${HAL_H264D_SRC})

target_link_libraries(hal_h264d vdpu34x_com mpp_base mpp_hal)
set_target_properties(hal_h264d PROPERTIES FOLDER "mpp/hal")

