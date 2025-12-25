# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)
include_directories(../../../codec/enc/h264/)
include_directories(../common/)

# hal h264 encoder sourse
add_library(hal_h264e_vpu OBJECT
    hal_h264e_vepu2_v2.c
    hal_h264e_vepu1_v2.c
    hal_h264e_vepu_v2.c
    hal_h264e_vpu_tbl.c
    )
