# vim: syntax=cmake
include_directories(.)
# hal/common for hal_bufs module
include_directories(../common/)
include_directories(../../common/h264/)
# mpp/codec/enc/h264 for h264 syntax
include_directories(../../../codec/enc/h264/)

# hal h264 header
set(HAL_H264E_HDR
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_vepu541.c
    hal_h264e_vepu580.c
    )

add_library(hal_h264e_rkv STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
            )

target_link_libraries(hal_h264e_rkv hal_vepu541_common hal_h264e hal_common)
set_target_properties(hal_h264e_rkv PROPERTIES FOLDER "mpp/hal")
