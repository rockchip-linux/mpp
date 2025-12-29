# vim: syntax=cmake
include_directories(.)
include_directories(../../rkenc/h264e/)
include_directories(../../vpu/h264e/)
include_directories(../../../codec/enc/h264/)

# hal h264 encoder common
add_library(hal_h264e_com OBJECT
    hal_h264e_stream_amend.c
    )
