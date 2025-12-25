# vim: syntax=cmake
include_directories(.)
# hal/common for hal_bufs module
include_directories(../common/)
include_directories(../../common/h264/)
# mpp/codec/enc/h264 for h264 syntax
include_directories(../../../codec/enc/h264/)

# hal h264 encoder sourse
add_library(hal_h264e_vepu5xx OBJECT
    hal_h264e_vepu541.c
    hal_h264e_vepu580.c
    hal_h264e_vepu540c.c
    hal_h264e_vepu510.c
    hal_h264e_vepu511.c
    )
