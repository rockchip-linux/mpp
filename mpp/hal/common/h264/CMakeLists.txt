# vim: syntax=cmake
include_directories(.)
include_directories(../../rkenc/h264e/)
include_directories(../../vpu/h264e/)
include_directories(../../../codec/enc/h264/)

# hal h264 encoder common
add_library(hal_h264e_com OBJECT
    hal_h264e_stream_amend.c
    )

# hal h264 encoder sourse
add_library(hal_h264e_api OBJECT
    hal_h264e_api_v2.c
    )
