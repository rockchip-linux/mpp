# vim: syntax=cmake


# hal h264 header
set(HAL_H264D_HDR
        hal_h264d_global.h
        hal_h264d_rkv_reg.h
        hal_h264d_vdpu_reg.h
        hal_h264d_vdpu1_reg.h
    )

# hal h264 decoder sourse
set(HAL_H264D_SRC
        hal_h264d_api.c
        hal_h264d_rkv_reg.c
        hal_h264d_vdpu_reg.c
        hal_h264d_vdpu1_reg.c
        hal_h264d_common.c
    )

add_library(hal_h264d STATIC
            ${HAL_H264D_HDR}
            ${HAL_H264D_SRC}
            )

target_link_libraries(hal_h264d mpp_base mpp_hal)
set_target_properties(hal_h264d PROPERTIES FOLDER "mpp/hal")

