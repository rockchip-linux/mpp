# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)
include_directories(../../../codec/enc/h264/)
include_directories(../common/)

# hal h264 header
set(HAL_H264E_HDR
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_vepu2_v2.c
    hal_h264e_vepu1_v2.c
    hal_h264e_vepu_v2.c
    hal_h264e_vpu_tbl.c
    )

add_library(hal_h264e_vpu STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
           )

target_link_libraries(hal_h264e_vpu hal_h264e hal_vepu_common ${CODEC_H264E})
set_target_properties(hal_h264e_vpu PROPERTIES FOLDER "mpp/hal")
